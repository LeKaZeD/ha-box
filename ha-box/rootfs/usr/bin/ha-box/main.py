#!/usr/bin/env python3
"""
HA Box - Main application entry point.

This is the main entry point for the HA Box App.
"""

import sys
import time
from pathlib import Path
from typing import Optional

# Add the ha-box package to the path
sys.path.insert(0, str(Path(__file__).parent))

from core import Config, load_options, setup_logging, parse_log_level, get_translator
from communication import ESP32Comm, Ack, ConnectionManager, ConnectionState, MessageHandler, KV
from communication.ota_manager import OTAManager, find_bundled_firmware
from api import HomeAssistantAPI
from handlers import SensorHandler, WeatherHandler, ClockHandler, StatusHandler, DayNightHandler, RF433Handler


# Setup logging
logger = setup_logging()

# Constants
INITIALIZATION_LOOP_INTERVAL = 5.0  # Check connection every 5 seconds during initialization
MAIN_LOOP_INTERVAL = 5.0  # Check disconnection every 5 seconds during normal operation
OTA_NOTICE_DISPLAY_DELAY_S = 2.5  # Let e-paper refresh OTA notice before flashing


def handle_esp32_ack(ack: Ack) -> None:
    """
    Handle ACK from ESP32.

    Args:
        ack: ACK received from ESP32.
    """
    if ack.ok:
        logger.debug("ACK OK received: id=%d", ack.id)
    else:
        logger.warning("ACK ERR received: id=%d, error=%s", ack.id, ack.err)


def _read_bundled_version() -> Optional[str]:
    """
    Read the bundled ESP32 firmware version from the firmware filename.

    Returns:
        Version string (e.g. "0.1.1") or None if not found.
    """
    path, version = find_bundled_firmware()
    if version is None:
        logger.warning("No bundled firmware file found (expected firmware-<version>.bin)")
    else:
        logger.info("Bundled firmware: %s (version %s)", path, version)
    return version


def _check_and_run_ota(
    message_handler: MessageHandler,
    ota_manager: Optional[OTAManager],
    esp32_comm: Optional["ESP32Comm"] = None,
    force_flash: bool = False,
) -> bool:
    """
    Compare ESP32 firmware version with bundled version and flash if different.

    Sends up to _OTA_VERSION_RETRIES VERSION queries to the ESP32, waiting
    _OTA_VERSION_TIMEOUT_S seconds for a reply each time. If the versions
    differ and OTA is available, flashes the firmware.

    Args:
        message_handler: Message handler (stores esp32_firmware_version).
        ota_manager: OTA manager (None if OTA is disabled).
        esp32_comm: ESP32 comm used to (re)send VERSION queries.

    Returns:
        True if OTA was performed (caller should re-enter init loop).
        False if no OTA was needed or OTA is not available.
    """
    _OTA_VERSION_TIMEOUT_S = 5.0
    _OTA_VERSION_RETRIES = 3

    if ota_manager is None or not ota_manager.is_available:
        return False

    bundled = _read_bundled_version()
    if bundled is None:
        logger.warning("Bundled firmware version unknown, skipping OTA check")
        return False

    # Query VERSION with retries: send the query, wait for a reply, repeat if needed.
    esp32_ver = None
    for attempt in range(1, _OTA_VERSION_RETRIES + 1):
        if message_handler.esp32_firmware_version is not None:
            break  # already received from a previous send (e.g. from _initialization_loop)

        if esp32_comm is not None:
            try:
                msg_id = esp32_comm.send_command("VERSION")
                logger.info(
                    "VERSION query sent (attempt %d/%d, id=%d)",
                    attempt, _OTA_VERSION_RETRIES, msg_id,
                )
            except Exception as e:
                logger.warning("Failed to send VERSION query (attempt %d): %s", attempt, e)

        deadline = time.time() + _OTA_VERSION_TIMEOUT_S
        while time.time() < deadline:
            if message_handler.esp32_firmware_version is not None:
                break
            time.sleep(0.25)

        if message_handler.esp32_firmware_version is not None:
            break

        logger.warning(
            "No VERSION reply (attempt %d/%d)",
            attempt, _OTA_VERSION_RETRIES,
        )

    esp32_ver = message_handler.esp32_firmware_version
    if esp32_ver is None:
        logger.warning(
            "ESP32 did not respond to VERSION after %d attempts, skipping OTA check",
            _OTA_VERSION_RETRIES,
        )
        return False

    logger.info("Version check: bundled=%s, esp32=%s", bundled, esp32_ver)

    if bundled == esp32_ver and not force_flash:
        logger.info("ESP32 firmware is up to date (%s)", esp32_ver)
        return False

    if force_flash and bundled == esp32_ver:
        logger.info("force_flash=True — bypassing version check, flashing anyway")
    else:
        logger.info(
            "Firmware mismatch (bundled=%s, esp32=%s): starting OTA flash",
            bundled, esp32_ver,
        )
    if esp32_comm is not None:
        try:
            esp32_comm.send_command("OTA")
            logger.info("OTA notice sent to ESP32 (show firmware update screen)")
            time.sleep(OTA_NOTICE_DISPLAY_DELAY_S)
        except Exception as e:
            logger.warning("Failed to send OTA notice to ESP32: %s", e)
    ok = ota_manager.flash()
    if ok:
        logger.info("OTA flash succeeded. Reconnecting to ESP32...")
        message_handler.esp32_firmware_version = None  # reset for next connection
    else:
        logger.error("OTA flash failed. Continuing with current firmware.")
    return ok


def _esp32_lang_id(config_lang: str) -> int:
    """ESP32 lang id: 0 = French, 1 = English."""
    if config_lang == "fr":
        return 0
    if config_lang == "en":
        return 1
    lang = get_translator().language
    return 0 if lang == "fr" else 1


def _initialization_loop(
    esp32_comm: ESP32Comm,
    connection_manager: ConnectionManager,
    message_handler: MessageHandler,
    weather_handler: WeatherHandler,
    clock_handler: ClockHandler,
    status_handler: StatusHandler,
    lang_config: str,
    fan_config=None,
    ota_manager: Optional[OTAManager] = None,
    daynight_handler: Optional[DayNightHandler] = None,
) -> None:
    """
    Initialization phase: wait for ESP32 connection.

    Actively tries to establish connection by sending READY messages periodically.
    If the ESP32 reports repeated boot failures (no valid firmware), triggers a
    blind OTA flash without waiting for a VERSION handshake.
    Once connected, initializes services and exits.

    Args:
        esp32_comm: ESP32 communication handler.
        connection_manager: Connection state manager.
        message_handler: Message handler for tracking last message time.
        weather_handler: Weather handler to initialize.
        clock_handler: Clock handler to initialize.
        lang_config: Language mode from config (auto, fr, en).
        fan_config: Optional fan configuration to send to the ESP32 on connect.
        ota_manager: Optional OTA manager for blind-flash recovery.
    """
    # Maximum blind-flash recovery attempts to avoid infinite loops.
    _BOOT_FAILURE_THRESHOLD = 3   # occurrences before triggering a flash
    _BOOT_RECOVERY_MAX_TRIES = 3  # max blind flash attempts per init phase

    boot_recovery_attempts = 0

    logger.info("Entering initialization phase: waiting for ESP32 connection...")

    while True:
        now = time.time()

        # Update connection state
        last_message_time = message_handler.last_message_time
        connection_manager.update(last_message_time, now)

        # Send periodic READY messages (connection_manager handles this)
        connection_manager.send_periodic_message(now)

        # Boot failure recovery: if the ESP32 is stuck in a reset loop with no
        # valid firmware, it sends ROM boot error messages on UART0.
        # Detect these and trigger a blind OTA flash to recover.
        if (
            ota_manager is not None
            and ota_manager.is_available
            and esp32_comm.get_boot_failure_count() >= _BOOT_FAILURE_THRESHOLD
            and boot_recovery_attempts < _BOOT_RECOVERY_MAX_TRIES
        ):
            boot_recovery_attempts += 1
            logger.warning(
                "ESP32 boot failure detected (%d patterns seen). "
                "Attempting blind OTA flash (attempt %d/%d)...",
                esp32_comm.get_boot_failure_count(),
                boot_recovery_attempts,
                _BOOT_RECOVERY_MAX_TRIES,
            )
            esp32_comm.reset_boot_failure_count()
            try:
                esp32_comm.send_command("OTA")
                logger.info("OTA notice sent to ESP32 before blind flash")
                time.sleep(OTA_NOTICE_DISPLAY_DELAY_S)
            except Exception as e:
                logger.warning("Failed to send OTA notice before blind flash: %s", e)
            ok = ota_manager.flash()
            if ok:
                logger.info("Blind OTA flash succeeded. Waiting for ESP32 to reboot...")
                message_handler.esp32_firmware_version = None
                connection_manager.state = ConnectionState.CONNECTING
                message_handler.last_message_time = 0.0
                try:
                    esp32_comm.send_command("READY")
                except Exception:
                    pass
            else:
                logger.error(
                    "Blind OTA flash failed (attempt %d/%d). "
                    "Will retry if boot failures continue.",
                    boot_recovery_attempts, _BOOT_RECOVERY_MAX_TRIES,
                )
            time.sleep(INITIALIZATION_LOOP_INTERVAL)
            continue

        # Check if connection is established
        if connection_manager.is_connected():
            logger.info("ESP32 connection established! Initializing services...")
            
            # Set connection check callbacks for handlers
            weather_handler.set_connection_check(connection_manager.is_connected)
            clock_handler.set_connection_check(connection_manager.is_connected)
            status_handler.set_connection_check(connection_manager.is_connected)
            if daynight_handler:
                daynight_handler.set_connection_check(connection_manager.is_connected)

            # Connection established: clear boot failure counter so it doesn't
            # carry over into the next initialization cycle.
            esp32_comm.reset_boot_failure_count()

            # Reset cached firmware version so the OTA check always uses a
            # freshly queried value, not one from a previous connection cycle.
            message_handler.esp32_firmware_version = None

            # Send language from App translator (ESP32: 0 = FR, 1 = EN; default EN if unsupported)
            lang_id = _esp32_lang_id(lang_config)
            try:
                msg_id = esp32_comm.send_command("LANG", [KV("id", str(lang_id))])
                logger.info("LANG sent to ESP32 (id=%d, lang_id=%d)", msg_id, lang_id)
            except Exception as e:
                logger.warning("Failed to send LANG: %s", e)

            # Send fan configuration (enable flag + temperature curve)
            if fan_config is not None:
                try:
                    msg_id = esp32_comm.send_command("FAN", [
                        KV("en",    "1" if fan_config.enabled else "0"),
                        KV("tOn",   str(fan_config.min_temp)),
                        KV("tFull", str(fan_config.max_temp)),
                    ])
                    logger.info(
                        "FAN config sent to ESP32 (id=%d, enabled=%s, tOn=%g, tFull=%g)",
                        msg_id, fan_config.enabled, fan_config.min_temp, fan_config.max_temp,
                    )
                except Exception as e:
                    logger.warning("Failed to send FAN config: %s", e)

            # Force initial updates (send immediately)
            logger.info("Sending initial data to ESP32...")
            weather_handler.force_update()
            clock_handler.force_update()
            status_handler.force_update()
            if daynight_handler:
                daynight_handler.esp32_comm = esp32_comm
                daynight_handler.force_update()

            # Start periodic updates (timers will handle subsequent updates)
            weather_handler.start_periodic_updates()
            clock_handler.start_periodic_updates()
            status_handler.start_periodic_updates()
            if daynight_handler:
                daynight_handler.start_periodic_updates()
            
            logger.info("Services initialized. Entering main operation phase...")
            return
        
        # Sleep before next check
        time.sleep(INITIALIZATION_LOOP_INTERVAL)


def _main_loop(
    esp32_comm: ESP32Comm,
    connection_manager: ConnectionManager,
    message_handler: MessageHandler,
    sensor_handler: SensorHandler,
    weather_handler: WeatherHandler,
    clock_handler: ClockHandler,
    status_handler: StatusHandler,
    daynight_handler: Optional[DayNightHandler] = None,
) -> bool:
    """
    Main operation phase: normal operation with periodic service updates.
    
    This loop monitors connection status and handles sensor data updates.
    Service updates (weather, clock) are handled by threading.Timer.
    If disconnection is detected, returns False to restart initialization.
    
    Args:
        esp32_comm: ESP32 communication handler.
        connection_manager: Connection state manager.
        message_handler: Message handler for tracking last message time.
        sensor_handler: Sensor handler for updating Home Assistant.
        weather_handler: Weather handler (timers handle updates).
        clock_handler: Clock handler (timers handle updates).
    
    Returns:
        True if should continue, False if should return to initialization.
    """
    logger.info("Entering main operation phase...")
    
    while True:
        now = time.time()
        
        # Update connection state
        last_message_time = message_handler.last_message_time
        connection_manager.update(last_message_time, now)
        
        # Send periodic messages (READY/STATUS) based on connection state
        connection_manager.send_periodic_message(now)
        
        # Check if connection is lost
        if not connection_manager.is_connected():
            logger.warning("ESP32 connection lost. Stopping services and returning to initialization...")
            
            # Stop periodic updates
            weather_handler.stop_periodic_updates()
            clock_handler.stop_periodic_updates()
            if status_handler:
                status_handler.stop_periodic_updates()
            if daynight_handler:
                daynight_handler.stop_periodic_updates()
            
            # Reset connection manager state to CONNECTING
            connection_manager.state = ConnectionState.CONNECTING
            
            return False  # Return to initialization
        
        # Update Home Assistant with latest sensor data (non-blocking)
        if sensor_handler:
            if sensor_handler.update_home_assistant():
                sensor_handler.clear_latest_data()
        
        # Sleep before next check (services are handled by timers)
        time.sleep(MAIN_LOOP_INTERVAL)


def main() -> None:
    """Main application entry point."""
    logger.info("HA Box starting...")
    
    esp32_comm: Optional[ESP32Comm] = None
    sensor_handler: Optional[SensorHandler] = None
    weather_handler: Optional[WeatherHandler] = None
    clock_handler: Optional[ClockHandler] = None
    status_handler: Optional[StatusHandler] = None
    daynight_handler: Optional[DayNightHandler] = None
    message_handler: Optional[MessageHandler] = None
    connection_manager: Optional[ConnectionManager] = None
    rf433_handler: Optional[RF433Handler] = None
    
    try:
        # Load configuration
        logger.info("Loading configuration...")
        options = load_options()
        config = Config.from_options(options)
        setup_logging(parse_log_level(config.box.log_level))
        logger.info("Configuration loaded (log_level=%s)", config.box.log_level)

        # Initialize Home Assistant API (needed to get Core language)
        logger.info("Initializing Home Assistant API...")
        ha_api = HomeAssistantAPI()
        logger.info("Home Assistant API client initialized")

        # Language: prefer Home Assistant Core default (Settings > System > General)
        ha_language = ha_api.get_core_language()
        translator = get_translator(ha_language)
        logger.info("Language: %s%s", translator.language, " (from HA Core)" if ha_language else " (from env/default)")
        
        # Initialize handlers
        logger.info("Initializing handlers...")
        sensor_handler = SensorHandler(ha_api)
        weather_handler = WeatherHandler(
            esp32_comm=None,  # Will be set after ESP32 init
            ha_api=ha_api,
            weather_entity=config.box.weather.weather_entity,
            update_interval=float(config.box.weather.update_interval),
        )
        clock_handler = ClockHandler(
            esp32_comm=None,  # Will be set after ESP32 init
            update_interval=float(config.box.clock.update_interval),
        )
        # RF433 handler (CC1101 over SPI + RFLink TCP server)
        rf433_cfg = config.box.rf433
        rf433_spi = (rf433_cfg.spi_bus, rf433_cfg.spi_device) if rf433_cfg.enabled else None
        if rf433_cfg.enabled:
            rf433_handler = RF433Handler(rf433_cfg, ha_api=ha_api)
            if rf433_handler.start():
                logger.info("RF433 handler started (port=%d)", rf433_cfg.tcp_port)
            else:
                logger.error("RF433 handler failed to start")
                rf433_handler = None
        else:
            logger.info("RF433 disabled (rf433.enabled=false)")

        status_handler = StatusHandler(
            esp32_comm=None,
            ha_api=ha_api,
            update_interval=float(config.box.status.update_interval),
            exposed_addon_slugs=frozenset(config.box.status.exposed_addon_slugs) or None,
            rf433_spi=rf433_spi,
        )
        daynight_handler = DayNightHandler(
            esp32_comm=None,
            ha_api=ha_api,
            update_interval=float(config.box.daynight.update_interval),
        )
        # ESP32 comm created before message_handler (needed for SHUTDOWN_ACCEPTED)
        uart_device = None if config.box.uart.device == "auto" else config.box.uart.device
        esp32_comm = ESP32Comm(device=uart_device)
        message_handler = MessageHandler(sensor_handler, ha_api=ha_api, esp32_comm=esp32_comm, daynight_handler=daynight_handler, status_handler=status_handler)

        # OTA manager (disabled if GPIO pins not configured)
        ota_config = config.box.ota
        ota_manager: Optional[OTAManager] = OTAManager(
            uart_device=esp32_comm._device,
            io0_gpio=ota_config.io0_gpio,
            en_gpio=ota_config.en_gpio,
            esp32_comm=esp32_comm,
        ) if ota_config.is_available else None
        if ota_manager:
            logger.info(
                "OTA enabled: io0_gpio=%d, en_gpio=%d",
                ota_config.io0_gpio, ota_config.en_gpio,
            )
        else:
            logger.info("OTA disabled (io0_gpio/en_gpio not configured)")
        
        # Initialize ESP32 communication
        logger.info("Initializing ESP32 communication...")
        try:
            esp32_comm.set_authorize_callback(message_handler.authorize_message)
            esp32_comm.set_message_callback(message_handler.handle_message)
            esp32_comm.set_ack_callback(handle_esp32_ack)
            esp32_comm.begin()
            logger.info("ESP32 communication initialized")
            
            # Update handlers with ESP32 comm
            weather_handler.esp32_comm = esp32_comm
            clock_handler.esp32_comm = esp32_comm
            status_handler.esp32_comm = esp32_comm
            
            # Initialize connection manager
            connection_manager = ConnectionManager(
                esp32_comm,
                connected_timeout=float(config.box.connection.connected_timeout),
                reconnecting_timeout=float(config.box.connection.reconnecting_timeout),
            )
            
            # Send initial READY message to ESP32
            time.sleep(0.5)  # Give ESP32 time to initialize
            logger.info("Sending initial READY message to ESP32...")
            try:
                msg_id = esp32_comm.send_command("READY")
                logger.info("READY message sent to ESP32 (id=%d)", msg_id)
            except Exception as e:
                logger.error("Failed to send READY: %s", e)
        except Exception as e:
            logger.error("Failed to initialize ESP32 communication: %s", e)
            logger.warning("Continuing without ESP32 communication")
            esp32_comm = None
        
        # Main application lifecycle
        if esp32_comm and connection_manager:
            try:
                while True:
                    # Phase 1: Initialization - wait for connection
                    _initialization_loop(
                        esp32_comm,
                        connection_manager,
                        message_handler,
                        weather_handler,
                        clock_handler,
                        status_handler,
                        lang_config=config.box.lang,
                        fan_config=config.box.fan,
                        ota_manager=ota_manager,
                        daynight_handler=daynight_handler,
                    )

                    # OTA check: flash if bundled version differs from ESP32 version.
                    # If OTA was performed, restart init loop to reconnect.
                    if _check_and_run_ota(message_handler, ota_manager, esp32_comm, force_flash=ota_config.force_flash):
                        ota_config.force_flash = False  # one-shot: don't re-flash on next reconnect
                        logger.info("OTA complete, restarting initialization loop...")
                        # Reset connection state: the ESP32 just rebooted from fresh
                        # firmware and is on the loading screen waiting for READY.
                        # Forcing CONNECTING state ensures _initialization_loop sends
                        # READY before anything else (LANG, FAN, etc.).
                        connection_manager.state = ConnectionState.CONNECTING
                        message_handler.last_message_time = 0.0
                        # Send an immediate READY so the ESP32 exits loading screen
                        # as soon as possible (don't wait for the periodic timer).
                        try:
                            esp32_comm.send_command("READY")
                            logger.info("READY sent to ESP32 after OTA reboot")
                        except Exception as e:
                            logger.warning("Failed to send READY after OTA: %s", e)
                        continue

                    # Phase 2: Main operation - normal operation
                    should_continue = _main_loop(
                        esp32_comm,
                        connection_manager,
                        message_handler,
                        sensor_handler,
                        weather_handler,
                        clock_handler,
                        status_handler,
                        daynight_handler=daynight_handler,
                    )
                    
                    # If main loop returns False, restart initialization
                    if not should_continue:
                        logger.info("Restarting initialization phase...")
                        continue
                    
            except KeyboardInterrupt:
                logger.info("Shutdown requested")
            finally:
                # Cleanup: stop all timers
                logger.info("Stopping services...")
                if weather_handler:
                    weather_handler.stop_periodic_updates()
                if clock_handler:
                    clock_handler.stop_periodic_updates()
                if status_handler:
                    status_handler.stop_periodic_updates()
                if daynight_handler:
                    daynight_handler.stop_periodic_updates()
                if rf433_handler:
                    rf433_handler.stop()
        else:
            logger.info("ESP32 communication not available. Exiting.")
        
    except Exception as e:
        logger.exception("Fatal error: %s", e)
        sys.exit(1)
    finally:
        # Final cleanup
        logger.info("Stopping ESP32 communication...")
        if esp32_comm:
            try:
                esp32_comm.stop()
            except Exception as e:
                logger.error("Error stopping ESP32 communication: %s", e)
        
        logger.info("HA Box stopped")


if __name__ == "__main__":
    main()
