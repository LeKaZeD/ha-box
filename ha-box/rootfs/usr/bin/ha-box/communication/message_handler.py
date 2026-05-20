"""
Message handler for ESP32 communication.

This module handles incoming messages from the ESP32 and routes them
to appropriate handlers.
"""

import logging
import time
from typing import TYPE_CHECKING, Optional, Tuple
from communication.esp32_comm import Message
from handlers.sensor_handler import SensorHandler

if TYPE_CHECKING:
    from api.ha_api import HomeAssistantAPI
    from communication.esp32_comm import ESP32Comm
    from handlers.daynight_handler import DayNightHandler
    from handlers.status_handler import StatusHandler

logger = logging.getLogger(__name__)


class MessageHandler:
    """Handler for routing ESP32 messages to appropriate processors."""
    
    def __init__(
        self,
        sensor_handler: Optional[SensorHandler] = None,
        ha_api: Optional["HomeAssistantAPI"] = None,
        esp32_comm: Optional["ESP32Comm"] = None,
        daynight_handler: Optional["DayNightHandler"] = None,
        status_handler: Optional["StatusHandler"] = None,
    ) -> None:
        """
        Initialize message handler.

        Args:
            sensor_handler: Optional sensor handler for SENS messages.
            ha_api: Optional HA API client (e.g. for ALL_OFF -> light.turn_off).
            esp32_comm: Optional ESP32 comm for sending (e.g. SHUTDOWN_ACCEPTED).
        """
        self.sensor_handler = sensor_handler
        self.ha_api = ha_api
        self.esp32_comm = esp32_comm
        self.daynight_handler = daynight_handler
        self.status_handler = status_handler
        self.last_message_time = 0.0
        self.esp32_firmware_version: Optional[str] = None
    
    def handle_message(self, msg: Message) -> None:
        """
        Handle incoming message from ESP32.
        
        Args:
            msg: Message received from ESP32.
        """
        kv_str = " ".join(f"{kv.key}={kv.val}" for kv in msg.kv) if msg.kv else ""
        if kv_str:
            logger.info("Received message from ESP32: verb=%s, id=%d, %s", msg.verb, msg.id, kv_str)
        else:
            logger.info("Received message from ESP32: verb=%s, id=%d", msg.verb, msg.id)

        # Update last message time
        self.last_message_time = time.time()
        
        # Route message to appropriate handler
        if msg.verb == "READY":
            logger.debug("ESP32 is ready")

        elif msg.verb == "VERSION":
            ver = msg.get("ver")
            if ver:
                self.esp32_firmware_version = ver
                logger.info("ESP32 firmware version: %s", ver)
            else:
                logger.warning("VERSION message missing ver key")
        
        elif msg.verb == "SENS":
            if self.sensor_handler:
                self.sensor_handler.handle_sens_message(msg)
            else:
                logger.warning("SENS message received but no sensor handler configured")
        
        elif msg.verb == "CASE":
            if self.sensor_handler:
                self.sensor_handler.handle_case_message(msg)
            else:
                logger.warning("CASE message received but no sensor handler configured")
        
        elif msg.verb == "STATUS":
            # STATUS is a heartbeat from ESP32 - just acknowledge receipt
            logger.debug("ESP32 STATUS heartbeat received")
        
        elif msg.verb == "TOUCH":
            # Handle touch events
            action = msg.get("action", "unknown")
            logger.info("Touch event: %s", action)
        
        elif msg.verb == "SHUTDOWN_REQUEST":
            logger.info("ESP32 requested host shutdown (power button short press)")
            if self.ha_api:
                if self.ha_api.request_host_shutdown():
                    logger.info("Host shutdown initiated successfully")
                    if self.esp32_comm:
                        self.esp32_comm.send_command("SHUTDOWN_ACCEPTED")
                        logger.info("SHUTDOWN_ACCEPTED sent to ESP32")
                    if self.status_handler:
                        self.status_handler.accelerate(5.0)
                        logger.info("Status heartbeat accelerated to 5s for shutdown detection")
                else:
                    logger.warning("Host shutdown request failed (check add-on hassio_role)")
            else:
                logger.warning("SHUTDOWN_REQUEST received but no HA API configured")

        elif msg.verb == "DAYMODE":
            mode_str = msg.get("mode") or "0"
            try:
                mode = int(mode_str)
            except ValueError:
                mode = 0
            if self.daynight_handler:
                self.daynight_handler.on_esp32_daymode(mode)
            else:
                logger.debug("DAYMODE received but no daynight handler configured")

        elif msg.verb == "ALL_OFF":
            logger.info("ESP32 requested all lights off")
            if self.ha_api:
                if self.ha_api.call_light_turn_off_all():
                    logger.info("All lights off executed successfully")
                else:
                    logger.warning("All lights off service call failed")
            else:
                logger.warning("ALL_OFF received but no HA API configured")

        else:
            logger.debug("Unknown message verb: %s", msg.verb)
    
    def authorize_message(self, msg: Message) -> Tuple[bool, Optional[str]]:
        """
        Authorize incoming message from ESP32.
        
        Args:
            msg: Message to authorize.
            
        Returns:
            Tuple of (authorized, error_code).
        """
        # Whitelist of allowed commands
        allowed_verbs = [
            "READY",
            "VERSION",
            "SENS",
            "CASE",
            "STATUS",
            "TOUCH",
            "DAYMODE",
            "SHUTDOWN_REQUEST",
            "ALL_OFF",
        ]
        
        if msg.verb in allowed_verbs:
            return (True, None)
        else:
            return (False, "unauthorized")
