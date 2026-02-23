"""
Clock handler for ESP32.

This module handles sending current time to the ESP32.
"""

import datetime
import logging
import threading
import time
from typing import Optional, Callable
from communication.esp32_comm import ESP32Comm, KV

logger = logging.getLogger(__name__)


class ClockHandler:
    """Handler for clock updates to ESP32."""
    
    def __init__(
        self,
        esp32_comm: Optional[ESP32Comm] = None,
        update_interval: float = 60.0
    ) -> None:
        """
        Initialize clock handler.
        
        Args:
            esp32_comm: ESP32 communication handler.
            update_interval: Update interval in seconds (default: 60 seconds).
        """
        self.esp32_comm = esp32_comm
        self.update_interval = update_interval
        self.last_update = 0.0
        self.timer: Optional[threading.Timer] = None
        self._is_connected_callback: Optional[Callable[[], bool]] = None
    
    def should_update(self, current_time: float) -> bool:
        """
        Check if clock should be updated.
        
        Args:
            current_time: Current timestamp.
            
        Returns:
            True if update is needed, False otherwise.
        """
        time_since_update = current_time - self.last_update
        should = time_since_update >= self.update_interval
        logger.debug("Clock should_update: time_since=%.1fs, interval=%.1fs, should=%s", 
                    time_since_update, self.update_interval, should)
        return should
    
    def set_connection_check(self, callback: Callable[[], bool]) -> None:
        """
        Set callback to check if ESP32 is connected.
        
        Args:
            callback: Function that returns True if connected, False otherwise.
        """
        self._is_connected_callback = callback
    
    def force_update(self) -> bool:
        """
        Force immediate clock update (for initialization).
        
        Returns:
            True if update was successful, False otherwise.
        """
        return self._send_clock()
    
    def start_periodic_updates(self) -> None:
        """Start periodic clock updates using threading.Timer."""
        logger.info("Starting periodic clock updates (interval=%.1fs)", self.update_interval)
        self._schedule_next()
    
    def stop_periodic_updates(self) -> None:
        """Stop periodic clock updates."""
        if self.timer:
            logger.info("Stopping periodic clock updates")
            self.timer.cancel()
            self.timer = None
    
    def _schedule_next(self) -> None:
        """Schedule next clock update."""
        if self.timer:
            self.timer.cancel()
        
        self.timer = threading.Timer(self.update_interval, self._periodic_update)
        self.timer.start()
        logger.debug("Clock update scheduled in %.1f seconds", self.update_interval)
    
    def _periodic_update(self) -> None:
        """
        Periodic update callback (called by timer).
        Checks connection before sending.
        """
        # Check if still connected
        if self._is_connected_callback and not self._is_connected_callback():
            logger.debug("Clock periodic update skipped: ESP32 not connected")
            # Don't schedule next update if disconnected
            return
        
        # Send clock update
        self._send_clock()
        
        # Schedule next update
        self._schedule_next()
    
    def _send_clock(self) -> bool:
        """
        Send current time to ESP32.
        
        Returns:
            True if update was successful, False otherwise.
        """
        logger.debug("ClockHandler._send_clock() called: esp32_comm=%s", self.esp32_comm is not None)
        
        if not self.esp32_comm:
            logger.warning("ClockHandler._send_clock() skipped: esp32_comm is None")
            return False
        
        try:
            dt = datetime.datetime.now()
            
            logger.info("Sending clock to ESP32: %02d:%02d:%02d (hh=%d, mm=%d, ss=%d)",
                        dt.hour, dt.minute, dt.second, dt.hour, dt.minute, dt.second)

            msg_id = self.esp32_comm.send_command("CLOCK", [
                KV("hh", str(dt.hour)),
                KV("mm", str(dt.minute)),
                KV("ss", str(dt.second)),
            ])
            
            logger.debug("Clock command sent with msg_id=%d", msg_id)
            
            self.last_update = time.time()
            return msg_id > 0
        except Exception as e:
            logger.error("Failed to send clock update: %s", e, exc_info=True)
            return False
    
    def update(self, current_time: float) -> bool:
        """
        Update clock (legacy method, kept for compatibility).
        
        Args:
            current_time: Current timestamp.
            
        Returns:
            True if update was successful, False otherwise.
        """
        if not self.should_update(current_time):
            return False
        return self._send_clock()