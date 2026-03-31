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

logger = logging.getLogger(__name__)


class MessageHandler:
    """Handler for routing ESP32 messages to appropriate processors."""
    
    def __init__(
        self,
        sensor_handler: Optional[SensorHandler] = None,
        ha_api: Optional["HomeAssistantAPI"] = None,
        esp32_comm: Optional["ESP32Comm"] = None,
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
        self.last_message_time = 0.0
    
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
        for i, kv in enumerate(msg.kv):
            logger.debug("  kv[%d]: %s=%s", i, kv.key, kv.val)
        
        # Update last message time
        old_time = self.last_message_time
        self.last_message_time = time.time()
        logger.debug("Updated last_message_time: %.1f → %.1f", old_time, self.last_message_time)
        
        # Route message to appropriate handler
        if msg.verb == "READY":
            logger.info("ESP32 is ready")
        
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
        
        elif msg.verb == "NFC":
            # Handle NFC events
            uid = msg.get("uid", "unknown")
            logger.info("NFC event: UID=%s", uid)
        
        elif msg.verb == "SHUTDOWN_REQUEST":
            logger.info("ESP32 requested host shutdown (power button short press)")
            if self.ha_api:
                if self.ha_api.request_host_shutdown():
                    logger.info("Host shutdown initiated successfully")
                    if self.esp32_comm:
                        self.esp32_comm.send_command("SHUTDOWN_ACCEPTED")
                        logger.debug("SHUTDOWN_ACCEPTED sent to ESP32 (immediate deep sleep)")
                else:
                    logger.warning("Host shutdown request failed (check add-on hassio_role)")
            else:
                logger.warning("SHUTDOWN_REQUEST received but no HA API configured")

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
            "SENS",
            "CASE",
            "STATUS",
            "TOUCH",
            "NFC",
            "SHUTDOWN_REQUEST",
            "ALL_OFF",
        ]
        
        if msg.verb in allowed_verbs:
            return (True, None)
        else:
            return (False, "unauthorized")
