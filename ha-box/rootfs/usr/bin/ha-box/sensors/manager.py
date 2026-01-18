"""Sensors manager."""

import logging
import threading
import time
from typing import Dict, Optional

from ha_box.config import Config
from ha_box.hal.i2c import I2CManager

logger = logging.getLogger(__name__)

class SensorsManager:
    """Manager for all sensors."""
    
    def __init__(self, config: Config, i2c_manager: I2CManager):
        """
        Initialize sensors manager.
        
        Args:
            config: Configuration object
            i2c_manager: I2C manager instance
        """
        self.config = config
        self.i2c_manager = i2c_manager
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._data: Dict = {}
        self._lock = threading.Lock()
        
        # Initialize sensor drivers (to be implemented in Phase 3)
        self._bme280 = None
        self._nfc = None
        self._touch = None
    
    def start(self):
        """Start sensors manager."""
        if self._running:
            logger.warning("Sensors manager already running")
            return
        
        logger.info("Starting sensors manager...")
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        logger.info("Sensors manager started")
    
    def stop(self):
        """Stop sensors manager."""
        if not self._running:
            return
        
        logger.info("Stopping sensors manager...")
        self._running = False
        if self._thread:
            self._thread.join(timeout=5)
        logger.info("Sensors manager stopped")
    
    def _run(self):
        """Main sensor reading loop."""
        while self._running:
            try:
                # Read sensors (to be implemented in Phase 3)
                data = {}
                
                # BME280
                if self.config.sensors.bme280.enabled:
                    # TODO: Implement BME280 reading in Phase 3
                    pass
                
                # NFC
                if self.config.sensors.nfc.enabled:
                    # TODO: Implement NFC reading in Phase 3
                    pass
                
                # Touch
                if self.config.sensors.touch.enabled:
                    # TODO: Implement touch reading in Phase 3
                    pass
                
                # Update data
                with self._lock:
                    self._data = data
                
                # Sleep
                time.sleep(1)
                
            except Exception as e:
                logger.error(f"Error in sensors loop: {e}")
                time.sleep(5)
    
    def get_data(self) -> Dict:
        """
        Get current sensor data.
        
        Returns:
            Dictionary with sensor data
        """
        with self._lock:
            return self._data.copy()
