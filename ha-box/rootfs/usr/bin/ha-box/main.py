#!/usr/bin/env python3
"""
HA Box - Main application entry point.

This is the main entry point for the HA Box add-on.
"""

import sys
import time
from pathlib import Path
from typing import Optional

# Add the ha-box package to the path
sys.path.insert(0, str(Path(__file__).parent))

from core import Config, load_options, setup_logging, get_translator
from communication import ESP32Comm, Ack, ConnectionManager, ConnectionState, MessageHandler, KV
from api import HomeAssistantAPI
from handlers import SensorHandler, WeatherHandler, ClockHandler

# Setup logging
logger = setup_logging()

# Constants
INITIALIZATION_LOOP_INTERVAL = 5.0  # Check connection every 5 seconds during initialization
MAIN_LOOP_INTERVAL = 5.0  # Check disconnection every 5 seconds during normal operation


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


def _esp32_lang_id() -> int:
    """ESP32 lang id from add-on translator: 0 = French, 1 = English (default if unsupported)."""
    lang = get_translator().language
    return 0 if lang == "fr" else 1


def _initialization_loop(
    esp32_comm: ESP32Comm,
    connection_manager: ConnectionManager,
    message_handler: MessageHandler,
    weather_handler: WeatherHandler,
    clock_handler: ClockHandler,
) -> None:
    """
    Initialization phase: wait for ESP32 connection.
    
    This loop actively tries to establish connection by sending READY messages
    periodically. Once connected, initializes services and exits.
    
    Args:
        esp32_comm: ESP32 communication handler.
        connection_manager: Connection state manager.
        message_handler: Message handler for tracking last message time.
        weather_handler: Weather handler to initialize.
        clock_handler: Clock handler to initialize.
    """
    logger.info("Entering initialization phase: waiting for ESP32 connection...")
    
    while True:
        now = time.time()
        
        # Update connection state
        last_message_time = message_handler.last_message_time
        connection_manager.update(last_message_time, now)
        
        # Send periodic READY messages (connection_manager handles this)
        connection_manager.send_periodic_message(now)
        
        # Check if connection is established
        if connection_manager.is_connected():
            logger.info("ESP32 connection established! Initializing services...")
            
            # Set connection check callbacks for handlers
            weather_handler.set_connection_check(connection_manager.is_connected)
            clock_handler.set_connection_check(connection_manager.is_connected)
            
            # Send language from add-on translator (ESP32: 0 = FR, 1 = EN; default EN if unsupported)
            lang_id = _esp32_lang_id()
            try:
                msg_id = esp32_comm.send_command("LANG", [KV("id", str(lang_id))])
                logger.info("LANG sent to ESP32 (id=%d, lang_id=%d)", msg_id, lang_id)
            except Exception as e:
                logger.warning("Failed to send LANG: %s", e)
            
            # Force initial updates (send immediately)
            logger.info("Sending initial data to ESP32...")
            weather_handler.force_update()
            clock_handler.force_update()
            
            # Start periodic updates (timers will handle subsequent updates)
            weather_handler.start_periodic_updates()
            clock_handler.start_periodic_updates()
            
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
    clock_handler: ClockHandler
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
            
            # Reset connection manager state to CONNECTING
            connection_manager.state = ConnectionState.CONNECTING
            
            return False  # Return to initialization
        
        # Update Home Assistant with latest sensor data (non-blocking)
        if sensor_handler:
            sensor_handler.update_home_assistant()
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
    message_handler: Optional[MessageHandler] = None
    connection_manager: Optional[ConnectionManager] = None
    
    try:
        # Load configuration
        logger.info("Loading configuration...")
        options = load_options()
        config = Config.from_options(options)
        logger.info("Configuration loaded")
        
        # Initialize translator
        translator = get_translator()
        logger.info("Language: %s", translator.language)
        
        # Initialize Home Assistant API
        logger.info("Initializing Home Assistant API...")
        ha_api = HomeAssistantAPI()
        logger.info("Home Assistant API client initialized")
        
        # Initialize handlers
        logger.info("Initializing handlers...")
        sensor_handler = SensorHandler(ha_api)
        weather_handler = WeatherHandler(
            esp32_comm=None,  # Will be set after ESP32 init
            ha_api=ha_api,
            weather_entity=config.home_assistant.weather_entity,
            update_interval=300.0  # 5 minutes default
        )
        clock_handler = ClockHandler(
            esp32_comm=None,  # Will be set after ESP32 init
            update_interval=60.0  # 60 seconds default
        )
        message_handler = MessageHandler(sensor_handler, ha_api=ha_api)
        
        # Initialize ESP32 communication
        logger.info("Initializing ESP32 communication...")
        try:
            esp32_comm = ESP32Comm()
            esp32_comm.set_authorize_callback(message_handler.authorize_message)
            esp32_comm.set_message_callback(message_handler.handle_message)
            esp32_comm.set_ack_callback(handle_esp32_ack)
            esp32_comm.begin()
            logger.info("ESP32 communication initialized")
            
            # Update handlers with ESP32 comm
            weather_handler.esp32_comm = esp32_comm
            clock_handler.esp32_comm = esp32_comm
            
            # Initialize connection manager
            connection_manager = ConnectionManager(esp32_comm)
            
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
                    )
                    
                    # Phase 2: Main operation - normal operation
                    should_continue = _main_loop(
                        esp32_comm,
                        connection_manager,
                        message_handler,
                        sensor_handler,
                        weather_handler,
                        clock_handler
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
        else:
            logger.warning("ESP32 communication not available. Exiting.")
        
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
