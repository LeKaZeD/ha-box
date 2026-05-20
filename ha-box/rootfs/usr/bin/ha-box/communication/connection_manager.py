"""
Connection state manager for ESP32 communication.

This module manages the connection state machine and periodic messaging.
"""

import logging
import threading
import time
from enum import Enum
from typing import Optional
from communication.esp32_comm import ESP32Comm

logger = logging.getLogger(__name__)


class ConnectionState(Enum):
    """Connection state with ESP32."""
    CONNECTING = "connecting"      # Initial connection, send READY every 5s
    CONNECTED = "connected"         # Active connection, send STATUS every 30s
    RECONNECTING = "reconnecting"   # Lost connection, send READY every 5s
    DISCONNECTED = "disconnected"   # No ESP32 detected, send READY every 10s


class ConnectionManager:
    """Manages ESP32 connection state and periodic messaging."""
    
    def __init__(
        self,
        esp32_comm: Optional[ESP32Comm] = None,
        connected_timeout: float = 120.0,
        reconnecting_timeout: float = 60.0
    ) -> None:
        """
        Initialize connection manager.
        
        Args:
            esp32_comm: ESP32 communication handler.
            connected_timeout: Timeout in seconds before transitioning to RECONNECTING.
            reconnecting_timeout: Timeout in seconds before transitioning to DISCONNECTED.
        """
        self.esp32_comm = esp32_comm
        self.state = ConnectionState.CONNECTING
        self.connected_timeout = connected_timeout
        self.reconnecting_timeout = reconnecting_timeout
        self._state_lock = threading.Lock()

        # State machine intervals (seconds)
        self.state_intervals = {
            ConnectionState.CONNECTING: 5.0,
            ConnectionState.CONNECTED: 30.0,
            ConnectionState.RECONNECTING: 5.0,
            ConnectionState.DISCONNECTED: 10.0,
        }

        self.last_message_time = time.time()
    
    def update(self, last_message_time: float, current_time: float) -> None:
        """
        Update connection state based on last message time.
        
        Args:
            last_message_time: Timestamp of last message from ESP32 (0.0 if never seen).
            current_time: Current timestamp.
        """
        # Calculate time since last message
        if last_message_time > 0:
            time_since_last_seen = current_time - last_message_time
        else:
            # Never seen ESP32
            time_since_last_seen = 999999.0

        with self._state_lock:
            old_state = self.state

            # State machine transitions
            if self.state == ConnectionState.CONNECTING:
                if time_since_last_seen < 30.0:
                    self.state = ConnectionState.CONNECTED
                    logger.info("ESP32 connection established")

            elif self.state == ConnectionState.CONNECTED:
                if time_since_last_seen > self.connected_timeout:
                    self.state = ConnectionState.RECONNECTING
                    logger.warning("ESP32 connection lost, attempting reconnection")

            elif self.state == ConnectionState.RECONNECTING:
                if time_since_last_seen < 30.0:
                    self.state = ConnectionState.CONNECTED
                    logger.info("ESP32 connection restored")
                elif time_since_last_seen > self.reconnecting_timeout:
                    self.state = ConnectionState.DISCONNECTED
                    logger.warning("ESP32 disconnected (no message for %.0fs)", time_since_last_seen)

            elif self.state == ConnectionState.DISCONNECTED:
                if time_since_last_seen < 30.0:
                    self.state = ConnectionState.CONNECTED
                    logger.info("ESP32 reconnected")

            # Log state change
            if old_state != self.state:
                logger.info("Connection state: %s → %s", old_state.value, self.state.value)
    
    def should_send_periodic_message(self, current_time: float) -> bool:
        """
        Check if periodic message should be sent.
        
        Args:
            current_time: Current timestamp.
            
        Returns:
            True if message should be sent, False otherwise.
        """
        interval = self.state_intervals.get(self.state, 30.0)
        return (current_time - self.last_message_time) >= interval
    
    def send_periodic_message(self, current_time: float) -> None:
        """
        Send periodic message based on current state.
        
        Args:
            current_time: Current timestamp.
        """
        if not self.esp32_comm:
            return

        if not self.should_send_periodic_message(current_time):
            return

        try:
            if self.state == ConnectionState.CONNECTED:
                # STATUS with optional KV is sent by StatusHandler; do not send here
                return
            # CONNECTING, RECONNECTING, DISCONNECTED: send READY
            logger.info("Sending READY to ESP32 (state: %s)", self.state.value)
            self.esp32_comm.send_command("READY")
            self.last_message_time = current_time
        except Exception as e:
            logger.error("Failed to send periodic message: %s", e, exc_info=True)
    
    def is_connected(self) -> bool:
        """
        Check if currently connected.

        Returns:
            True if connected, False otherwise.
        """
        with self._state_lock:
            return self.state == ConnectionState.CONNECTED
