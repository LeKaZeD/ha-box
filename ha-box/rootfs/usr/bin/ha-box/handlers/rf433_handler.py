"""
RF433 handler — RFLink-compatible TCP server backed by CC1101 (SPI on Pi).

The CC1101 is driven directly via spidev (no external CC1101 library needed).
Configuration: 433.92 MHz, OOK/ASK, variable packet length, no sync word,
no CRC — "raw" capture mode so any OOK device on 433 MHz is received.

Protocol (text over TCP, line-delimited by \\r\\n):
    RX  (CC1101 → TCP clients) : 20;SEQ;RAW;DATA=hexhex;\\r\\n
    TX  (TCP clients → CC1101) : 10;Protocol;ID;[DATA=hexhex;]
    PING→PONG                  : 20;01;MySensors_Gateway;ID=0000;MSG=PONG;\\r\\n

Point the HA RFLink integration at localhost:<tcp_port>.
"""

import logging
import socket
import threading
import time
from typing import TYPE_CHECKING, List, Optional, Tuple

from core.config import RF433Config

if TYPE_CHECKING:
    from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Minimal CC1101 driver (spidev only — no external dependency)
# ---------------------------------------------------------------------------

class _CC1101:
    """
    Minimal CC1101 driver over SPI for 433.92 MHz OOK reception/transmission.

    Register values target a 26 MHz crystal (standard CC1101 modules).
    Data rate ≈ 4.8 kbps, channel BW ≈ 325 kHz — covers most OOK remotes.
    """

    # ---- Command strobes (write single byte) ----
    _SRES  = 0x30  # Software reset
    _SCAL  = 0x33  # Calibrate frequency synthesizer
    _SRX   = 0x34  # Enable RX
    _STX   = 0x35  # Enable TX (from IDLE, FSTXON)
    _SIDLE = 0x36  # Exit RX/TX, turn off FS
    _SFRX  = 0x3A  # Flush RX FIFO
    _SFTX  = 0x3B  # Flush TX FIFO

    # ---- Status registers (read-only, burst flag | R flag | addr) ----
    _RXBYTES   = 0xFB  # Number of bytes in RX FIFO
    _TXBYTES   = 0xFA  # Number of bytes in TX FIFO
    _MARCSTATE = 0xF5  # Main radio control state machine

    # ---- Configuration for 433.92 MHz OOK, raw variable-length packets ----
    #  Crystal: 26 MHz
    #  Frequency: 433.92 MHz  →  FREQ = round(433.92e6 / 26e6 * 2**16) = 0x10B071
    #  Modulation: OOK/ASK
    #  Data rate: ~4.8 kbps  (DRATE_E=8, DRATE_M=0x3B → 4800 bps)
    #  Channel BW: 325 kHz   (CHANBW_E=1, CHANBW_M=0)
    #  Packet: variable length, no whitening, no CRC, no sync word
    _INIT_REGS: List[Tuple[int, int]] = [
        (0x00, 0x0D),  # IOCFG0   : GDO0 = assert on sync / de-assert at end of packet
        (0x02, 0x29),  # IOCFG2   : GDO2 = FIFO threshold or end of packet
        (0x03, 0x47),  # FIFOTHR  : RX threshold = 32 bytes
        (0x06, 0xFF),  # PKTLEN   : max packet length (variable mode)
        (0x07, 0x04),  # PKTCTRL1 : no address check, append status off
        (0x08, 0x02),  # PKTCTRL0 : infinite packet length, no whitening, no CRC
        (0x0B, 0x06),  # FSCTRL1  : IF = 152 kHz
        (0x0D, 0x10),  # FREQ2    : 433.92 MHz
        (0x0E, 0xB0),  # FREQ1
        (0x0F, 0x71),  # FREQ0
        (0x10, 0x68),  # MDMCFG4  : CHANBW_E=1, CHANBW_M=2, DRATE_E=8 → BW≈203 kHz
        (0x11, 0x93),  # MDMCFG3  : DRATE_M=0x93 → ~9.6 kbps
        (0x12, 0x30),  # MDMCFG2  : OOK, no Manchester, no sync word
        (0x13, 0x22),  # MDMCFG1  : no FEC, 4 preamble bytes
        (0x14, 0xF8),  # MDMCFG0  : default channel spacing
        (0x15, 0x34),  # DEVIATN  : (FSK only, irrelevant for OOK)
        (0x17, 0x3F),  # MCSM1    : stay RX after packet; go IDLE after TX
        (0x18, 0x18),  # MCSM0    : auto-calibrate on IDLE→RX/TX
        (0x19, 0x16),  # FOCCFG
        (0x1B, 0xAD),  # AGCCTRL2 : OOK optimised
        (0x1C, 0x00),  # AGCCTRL1
        (0x1D, 0xB2),  # AGCCTRL0
        (0x21, 0x56),  # FREND1
        (0x22, 0x11),  # FREND0   : PA table index 1 for OOK '1'
        (0x23, 0xE9),  # FSCAL3
        (0x24, 0x2A),  # FSCAL2
        (0x25, 0x00),  # FSCAL1
        (0x26, 0x1F),  # FSCAL0
    ]

    # PA table: [off (OOK '0'), +10 dBm (OOK '1')]
    _PA_TABLE = [0x00, 0xC0]

    def __init__(self, bus: int, device: int) -> None:
        import spidev  # type: ignore
        self._spi = spidev.SpiDev()
        self._spi.open(bus, device)
        self._spi.max_speed_hz = 50_000
        self._spi.mode = 0

    def reset(self) -> None:
        self._strobe(self._SRES)
        time.sleep(0.01)

    def configure(self) -> None:
        """Write register configuration and PA table."""
        for addr, val in self._INIT_REGS:
            self._write_reg(addr, val)
        # Burst-write PA table (addr 0x3E with burst flag 0x40)
        self._spi.xfer2([0x3E | 0x40] + self._PA_TABLE)

    def rx_mode(self) -> None:
        """Put CC1101 in receive mode."""
        self._strobe(self._SIDLE)
        self._strobe(self._SFRX)
        self._strobe(self._SRX)

    def recv(self, timeout: float = 1.0) -> Optional[bytes]:
        """
        Poll RX FIFO until bytes are available or timeout expires.

        In infinite packet mode the FIFO fills with raw received bytes — no
        length byte is prepended by hardware, so we read whatever is present.
        Returns the raw bytes or None on timeout / overflow.
        """
        deadline = time.time() + timeout
        while time.time() < deadline:
            n_raw = self._read_status_reg(self._RXBYTES)
            n = n_raw & 0x7F
            overflow = bool(n_raw & 0x80)
            if overflow:
                logger.debug("CC1101 RX FIFO overflow — flushing")
                self.rx_mode()
                return None
            if n > 0:
                # Burst-read n bytes from RX FIFO (addr 0xFF = 0x3F|READ|BURST)
                raw = self._spi.xfer2([0xFF] + [0x00] * n)
                return bytes(raw[1:])
            time.sleep(0.005)
        return None

    def send(self, data: List[int]) -> None:
        """Transmit a list of bytes via CC1101."""
        if not data:
            return
        self._strobe(self._SIDLE)
        self._strobe(self._SFTX)
        # Burst-write TX FIFO: length byte + data (addr 0x3F with burst flag 0x40)
        self._spi.xfer2([0x3F | 0x40, len(data)] + data)
        self._strobe(self._STX)
        # Wait for TX to complete (MARCSTATE → IDLE = 0x01)
        deadline = time.time() + 2.0
        while time.time() < deadline:
            if (self._read_status_reg(self._MARCSTATE) & 0x1F) == 0x01:
                break
            time.sleep(0.005)
        self.rx_mode()

    def close(self) -> None:
        try:
            self._strobe(self._SIDLE)
        except Exception:
            pass
        try:
            self._spi.close()
        except Exception:
            pass

    # ---- low-level helpers ----

    def _strobe(self, cmd: int) -> None:
        self._spi.xfer2([cmd])

    def _write_reg(self, addr: int, val: int) -> None:
        self._spi.xfer2([addr & 0x3F, val])

    def _read_status_reg(self, addr: int) -> int:
        """Read a status register (R=1, BURST=1 → addr | 0xC0)."""
        return self._spi.xfer2([addr | 0xC0, 0x00])[1]


# ---------------------------------------------------------------------------
# RF433Handler — RFLink TCP server
# ---------------------------------------------------------------------------

class RF433Handler:
    """CC1101 driver exposing a RFLink-compatible TCP server."""

    _SEQ_MAX = 255

    def __init__(self, config: RF433Config, ha_api: Optional["HomeAssistantAPI"] = None) -> None:
        self._config = config
        self._ha_api = ha_api
        self._server_sock: Optional[socket.socket] = None
        self._clients: List[socket.socket] = []
        self._clients_lock = threading.Lock()
        self._running = False
        self._seq = 1
        self._cc1101: Optional[_CC1101] = None
        self._rx_thread: Optional[threading.Thread] = None
        self._accept_thread: Optional[threading.Thread] = None

    # ------------------------------------------------------------------ public

    def start(self) -> bool:
        """Initialise CC1101 and start TCP server + RX/accept threads."""
        if not self._init_cc1101():
            return False
        if not self._start_server():
            return False

        self._running = True

        self._rx_thread = threading.Thread(
            target=self._rx_loop, name="rf433-rx", daemon=True
        )
        self._rx_thread.start()

        self._accept_thread = threading.Thread(
            target=self._accept_loop, name="rf433-accept", daemon=True
        )
        self._accept_thread.start()

        logger.info(
            "RF433 RFLink TCP server listening on :%d (SPI%d.%d gdo0=%d gdo2=%d)",
            self._config.tcp_port,
            self._config.spi_bus,
            self._config.spi_device,
            self._config.gdo0_pin,
            self._config.gdo2_pin,
        )
        return True

    def stop(self) -> None:
        """Stop the TCP server and all background threads."""
        self._running = False
        if self._server_sock:
            try:
                self._server_sock.close()
            except Exception:
                pass
            self._server_sock = None
        with self._clients_lock:
            for c in self._clients:
                try:
                    c.close()
                except Exception:
                    pass
            self._clients.clear()
        if self._cc1101:
            self._cc1101.close()
            self._cc1101 = None
        logger.info("RF433 handler stopped")

    # ----------------------------------------------------------------- private

    def _init_cc1101(self) -> bool:
        try:
            cc = _CC1101(self._config.spi_bus, self._config.spi_device)
            cc.reset()
            cc.configure()
            cc.rx_mode()
            self._cc1101 = cc
            logger.info(
                "CC1101 initialised on SPI%d.%d gdo0=%d @ 433.92 MHz OOK",
                self._config.spi_bus,
                self._config.spi_device,
                self._config.gdo0_pin,
            )
            return True
        except ImportError:
            logger.error("spidev not installed — RF433 disabled")
        except Exception as e:
            logger.error("CC1101 init failed: %s", e)
        return False

    def _start_server(self) -> bool:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind(("0.0.0.0", self._config.tcp_port))
            sock.listen(5)
            sock.settimeout(1.0)
            self._server_sock = sock
            return True
        except Exception as e:
            logger.error(
                "RF433 TCP server bind failed on port %d: %s",
                self._config.tcp_port, e,
            )
            return False

    def _accept_loop(self) -> None:
        while self._running and self._server_sock:
            try:
                conn, addr = self._server_sock.accept()
                logger.info("RFLink client connected: %s", addr)
                conn.settimeout(None)
                with self._clients_lock:
                    self._clients.append(conn)
                threading.Thread(
                    target=self._client_loop, args=(conn, addr), daemon=True
                ).start()
            except socket.timeout:
                continue
            except Exception as e:
                if self._running:
                    logger.warning("Accept error: %s", e)

    def _client_loop(self, conn: socket.socket, addr: Tuple) -> None:
        buf = b""
        try:
            while self._running:
                chunk = conn.recv(256)
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    self._handle_tx(line.decode(errors="replace").strip())
        except Exception as e:
            if self._running:
                logger.debug("Client %s read error: %s", addr, e)
        finally:
            with self._clients_lock:
                if conn in self._clients:
                    self._clients.remove(conn)
            try:
                conn.close()
            except Exception:
                pass
            logger.info("RFLink client disconnected: %s", addr)

    @staticmethod
    def _is_noise(pkt: bytes) -> bool:
        """True if the packet is ambient OOK carrier with no useful signal.

        A real OOK signal has alternating carrier (0xFF) and silence (0x00)
        bursts. Pure ambient noise is almost entirely 0xFF with at most a few
        bit-flips. We require at least 15% of bytes to be "silence" (value
        below 0x10) to consider the packet a real signal.
        """
        if not pkt:
            return True
        silence = sum(1 for b in pkt if b < 0x10)
        return (silence / len(pkt)) < 0.15

    def _rx_loop(self) -> None:
        if self._cc1101 is None:
            return
        while self._running:
            try:
                pkt = self._cc1101.recv(timeout=1.0)
                if pkt and not self._is_noise(pkt):
                    self._broadcast(self._to_rflink(pkt))
                    if self._ha_api:
                        try:
                            self._ha_api.fire_event("ha_box_rf433_received", {"data": pkt.hex()})
                        except Exception as e:
                            logger.debug("RF433 HA event failed: %s", e)
            except Exception as e:
                logger.debug("RX error: %s", e)
                time.sleep(0.1)

    def _handle_tx(self, line: str) -> None:
        if not line:
            return
        if line.startswith("10;PING"):
            self._broadcast("20;01;MySensors_Gateway;ID=0000;MSG=PONG;\r\n")
            return
        parts = [p.strip() for p in line.split(";") if p.strip()]
        if not parts or parts[0] != "10":
            logger.debug("Ignoring non-TX line: %s", line)
            return
        data_bytes: Optional[bytes] = None
        for part in parts[2:]:
            if part.upper().startswith("DATA="):
                try:
                    data_bytes = bytes.fromhex(part[5:])
                except ValueError:
                    logger.warning("Invalid DATA hex in TX command: %s", part)
        if data_bytes and self._cc1101:
            try:
                self._cc1101.send(list(data_bytes))
                logger.debug("TX %d bytes: %s", len(data_bytes), data_bytes.hex())
            except Exception as e:
                logger.warning("CC1101 TX failed: %s", e)
        else:
            logger.debug("TX command without DATA= field (ignored): %s", line)

    def _to_rflink(self, pkt: bytes) -> str:
        return f"20;{self._next_seq():02X};RAW;DATA={pkt.hex()};\r\n"

    def _broadcast(self, line: str) -> None:
        data = line.encode() if isinstance(line, str) else line
        dead: List[socket.socket] = []
        with self._clients_lock:
            for c in self._clients:
                try:
                    c.sendall(data)
                except Exception:
                    dead.append(c)
            for c in dead:
                self._clients.remove(c)
                try:
                    c.close()
                except Exception:
                    pass

    def _next_seq(self) -> int:
        seq = self._seq
        self._seq = (self._seq % self._SEQ_MAX) + 1
        return seq
