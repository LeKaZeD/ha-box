"""
ESP32 OTA (Over-The-Air) firmware update manager.

Flashes new firmware to the ESP32 via UART0 (bootloader mode).
The Pi controls IO0 and EN GPIO pins to enter/exit bootloader mode,
then runs esptool to write the firmware binary.

Hardware requirements:
  - UART0 (GPIO1/GPIO3 on ESP32) connected to Pi UART (same port used for normal comm)
  - ESP32 IO0 (GPIO0) connected to a Pi GPIO output
  - ESP32 EN (enable/reset) connected to a Pi GPIO output

Bootloader entry sequence:
  1. Assert IO0 LOW  (hold ESP32 in bootloader mode on next reset)
  2. Pulse EN LOW → HIGH  (reset ESP32 → boots into bootloader)
  3. Run esptool on the UART port
  4. Release IO0 HIGH
  5. Pulse EN LOW → HIGH  (reset ESP32 → boots into new firmware)
"""

import contextlib
import glob
import io
import logging
import os
import re
import time
from typing import Optional, Tuple, TYPE_CHECKING

if TYPE_CHECKING:
    from communication.esp32_comm import ESP32Comm

logger = logging.getLogger(__name__)

FIRMWARE_DIR = "/usr/share/ha-box/firmware"
BUNDLED_FLASH_ARGS = "/usr/share/ha-box/firmware/flash_args"

_FIRMWARE_RE = re.compile(r"firmware-(.+)\.bin$")


def find_bundled_firmware() -> Tuple[Optional[str], Optional[str]]:
    """Find the bundled firmware binary by globbing the firmware directory.

    Returns:
        (firmware_path, version) tuple, or (None, None) if not found.
    """
    matches = glob.glob(os.path.join(FIRMWARE_DIR, "firmware-*.bin"))
    if not matches:
        logger.warning("No firmware-*.bin found in %s", FIRMWARE_DIR)
        return None, None
    path = matches[0]
    m = _FIRMWARE_RE.search(os.path.basename(path))
    version = m.group(1) if m else None
    logger.info("Bundled firmware: %s (version %s)", path, version)
    return path, version

# esptool flash parameters (from flash_args).
FLASH_MODE = "dio"
FLASH_FREQ = "80m"
FLASH_SIZE = "4MB"
FLASH_ADDRESS = "0x10000"
FLASH_BAUDRATE = "460800"

# GPIO pulse durations (seconds).
_EN_PULSE_S = 0.1       # EN LOW pulse duration to trigger reset
_IO0_SETTLE_S = 0.05    # settle time after asserting IO0


class OTAManager:
    """
    Manages OTA firmware updates for the ESP32 via UART bootloader.

    Usage:
        manager = OTAManager(
            uart_device="/dev/serial0",
            io0_gpio=24,   # Pi GPIO BCM number connected to ESP32 IO0
            en_gpio=25,    # Pi GPIO BCM number connected to ESP32 EN
        )
        success = manager.flash()
    """

    def __init__(
        self,
        uart_device: str,
        io0_gpio: Optional[int],
        en_gpio: Optional[int],
        firmware_path: Optional[str] = None,
        esp32_comm: Optional["ESP32Comm"] = None,
    ) -> None:
        """
        Initialize OTA manager.

        Args:
            uart_device: UART device path (e.g. /dev/serial0).
            io0_gpio: BCM GPIO pin number connected to ESP32 IO0 (bootloader select).
                      None disables OTA.
            en_gpio: BCM GPIO pin number connected to ESP32 EN (reset).
                     None disables OTA.
            firmware_path: Path to the firmware .bin file.
            esp32_comm: Optional ESP32Comm instance to stop/restart around flash.
        """
        self._uart_device = uart_device
        self._io0_gpio = io0_gpio
        self._en_gpio = en_gpio
        self._firmware_path = firmware_path
        self._esp32_comm = esp32_comm
        self._gpio_chip = None
        self._io0_line = None
        self._en_line = None

    @property
    def is_available(self) -> bool:
        """True if OTA is configured (GPIO pins are set)."""
        return self._io0_gpio is not None and self._en_gpio is not None

    def flash(self) -> bool:
        """
        Flash the bundled firmware to the ESP32.

        Stops the ESP32 comm, enters bootloader mode via GPIO, runs esptool,
        then resets the ESP32 into normal operation and restarts comm.

        Returns:
            True if flash succeeded, False otherwise.
        """
        if not self.is_available:
            logger.warning("OTA not available: io0_gpio or en_gpio not configured")
            return False

        firmware_path = self._firmware_path
        if firmware_path is None:
            firmware_path, _ = find_bundled_firmware()
        if firmware_path is None or not os.path.isfile(firmware_path):
            logger.error("Firmware not found: %s", firmware_path)
            return False

        logger.info(
            "Starting OTA flash: firmware=%s, uart=%s, io0_gpio=%d, en_gpio=%d",
            firmware_path, self._uart_device, self._io0_gpio, self._en_gpio,
        )

        comm_was_running = False
        if self._esp32_comm and self._esp32_comm._running:
            logger.info("Stopping ESP32 comm to release UART for esptool")
            self._esp32_comm.stop()
            comm_was_running = True
            # Give the kernel time to fully release the serial fd and clear
            # any exclusive-mode (TIOCEXCL) state before esptool opens it.
            time.sleep(1.0)

        try:
            ok = self._do_flash(firmware_path)
        finally:
            if comm_was_running and self._esp32_comm:
                logger.info("Restarting ESP32 comm after OTA")
                try:
                    self._esp32_comm.begin()
                except Exception as e:
                    logger.error("Failed to restart ESP32 comm: %s", e)

        return ok

    def _do_flash(self, firmware_path: str) -> bool:
        """
        Internal flash sequence: GPIO bootloader entry + esptool + GPIO reboot.

        Returns:
            True if esptool exited successfully.
        """
        try:
            import gpiod  # type: ignore
        except ImportError:
            logger.error(
                "gpiod Python module not available. "
                "Install it with: pip install gpiod"
            )
            return False

        chip_path = "/dev/gpiochip0"
        request = None

        try:
            chip = gpiod.Chip(chip_path)
            request = chip.request_lines(
                consumer="ha-box-ota",
                config={
                    self._io0_gpio: gpiod.LineSettings(
                        direction=gpiod.line.Direction.OUTPUT,
                        output_value=gpiod.line.Value.ACTIVE,  # start HIGH (normal)
                    ),
                    self._en_gpio: gpiod.LineSettings(
                        direction=gpiod.line.Direction.OUTPUT,
                        output_value=gpiod.line.Value.ACTIVE,  # start HIGH (running)
                    ),
                },
            )

            # Step 1: Assert IO0 LOW (bootloader mode on next reset)
            logger.debug("IO0 → LOW (bootloader select)")
            request.set_value(self._io0_gpio, gpiod.line.Value.INACTIVE)
            time.sleep(_IO0_SETTLE_S)

            # Step 2: Pulse EN LOW → HIGH (reset into bootloader)
            logger.debug("EN → LOW (reset pulse)")
            request.set_value(self._en_gpio, gpiod.line.Value.INACTIVE)
            time.sleep(_EN_PULSE_S)
            logger.debug("EN → HIGH (bootloader running)")
            request.set_value(self._en_gpio, gpiod.line.Value.ACTIVE)

            # Give bootloader time to initialize (~200ms for ESP32 ROM)
            time.sleep(0.3)

            # Step 3: Flash via esptool
            logger.info("Running esptool...")
            ok = self._run_esptool(firmware_path)

            # Step 4: Release IO0 HIGH (normal boot mode)
            logger.debug("IO0 → HIGH (normal mode)")
            request.set_value(self._io0_gpio, gpiod.line.Value.ACTIVE)
            time.sleep(_IO0_SETTLE_S)

            # Step 5: Pulse EN to reboot into new firmware
            logger.debug("EN → LOW then HIGH (reboot into new firmware)")
            request.set_value(self._en_gpio, gpiod.line.Value.INACTIVE)
            time.sleep(_EN_PULSE_S)
            request.set_value(self._en_gpio, gpiod.line.Value.ACTIVE)

            logger.info("OTA flash %s", "succeeded" if ok else "failed")
            return ok

        except Exception as e:
            logger.error("OTA GPIO error: %s", e, exc_info=True)
            return False
        finally:
            if request:
                request.release()

    def _run_esptool(self, firmware_path: str) -> bool:
        """
        Run esptool in-process to write the firmware at 0x10000.

        Uses esptool's Python API directly (no subprocess) so we can patch
        pyserial to disable exclusive locking (TIOCEXCL). The HA add-on
        container does not have the CAP_SYS_TTY_CONFIG capability required to
        acquire exclusive mode on a tty when another fd exists on the host.

        Returns:
            True if esptool succeeded, False otherwise.
        """
        try:
            import esptool  # type: ignore
            import serial as _serial # type: ignore
        except ImportError as e:
            logger.error("Required module not available: %s", e)
            return False

        args = [
            "--chip", "esp32",
            "--port", self._uart_device,
            "--baud", FLASH_BAUDRATE,
            "--before", "no-reset",
            "--after", "no-reset",
            "write-flash",
            "--flash-mode", FLASH_MODE,
            "--flash-freq", FLASH_FREQ,
            "--flash-size", FLASH_SIZE,
            FLASH_ADDRESS, firmware_path,
        ]
        logger.info("esptool args: %s", " ".join(args))

        # Patch pyserial to force exclusive=False.
        # esptool v5+ sets exclusive=True by default on Linux, which calls
        # TIOCEXCL. This ioctl fails (EACCES) in Docker when another process
        # (e.g. a host getty or HAOS service) has the tty open without exclusive
        # mode, because the container lacks CAP_SYS_TTY_CONFIG.
        _orig_serial_init = _serial.Serial.__init__

        def _no_exclusive_init(self_s, *args_s, **kwargs_s):
            # Remove 'exclusive' so pyserial defaults to None.
            # exclusive=None → pyserial skips both TIOCEXCL and fcntl.flock(),
            # which avoids permission errors in the Docker container.
            # (exclusive=False would still trigger flock(LOCK_UN), which AppArmor
            # blocks because the serial device rules lack the 'k' permission.)
            kwargs_s.pop("exclusive", None)
            _orig_serial_init(self_s, *args_s, **kwargs_s)

        _serial.Serial.__init__ = _no_exclusive_init  # type: ignore[method-assign]

        stdout_buf = io.StringIO()
        stderr_buf = io.StringIO()
        exit_code = -1

        try:
            with contextlib.redirect_stdout(stdout_buf), \
                 contextlib.redirect_stderr(stderr_buf):
                esptool.main(args)
            exit_code = 0
        except SystemExit as e:
            exit_code = e.code if isinstance(e.code, int) else (-1 if e.code else 0)
        except Exception as e:
            logger.error("esptool raised an exception: %s", e, exc_info=True)
            exit_code = -1
        finally:
            _serial.Serial.__init__ = _orig_serial_init

        for line in stdout_buf.getvalue().splitlines():
            if line.strip():
                logger.info("[esptool] %s", line)
        for line in stderr_buf.getvalue().splitlines():
            if line.strip():
                logger.warning("[esptool stderr] %s", line)

        if exit_code == 0:
            logger.info("esptool succeeded (exit 0)")
            return True
        else:
            logger.error("esptool failed (exit %d)", exit_code)
            return False
