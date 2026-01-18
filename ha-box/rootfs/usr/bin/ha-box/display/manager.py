"""Display manager for E-Paper screen."""

import logging
from typing import Dict, List, Optional

from ha_box.config import Config
from ha_box.hal.spi import SPIManager
from ha_box.hal.gpio import GPIOManager

logger = logging.getLogger(__name__)

class DisplayManager:
    """Manager for E-Paper display."""
    
    def __init__(self, config: Config, spi_manager: SPIManager, gpio_manager: GPIOManager):
        """
        Initialize display manager.
        
        Args:
            config: Configuration object
            spi_manager: SPI manager instance
            gpio_manager: GPIO manager instance
        """
        self.config = config
        self.spi_manager = spi_manager
        self.gpio_manager = gpio_manager
        self._initialized = False
        
        # GPIO pins for display control (to be configured)
        self._dc_pin: Optional[int] = None
        self._reset_pin: Optional[int] = None
        self._busy_pin: Optional[int] = None
        self._front_light_pin: Optional[int] = None
    
    def start(self):
        """Start display manager."""
        if not self.config.display.enabled:
            logger.info("Display disabled in configuration")
            return
        
        logger.info("Initializing display...")
        
        # TODO: Initialize E-Paper display in Phase 3
        # - Setup GPIO pins
        # - Initialize UC8253 controller
        # - Display boot screen
        
        self._initialized = True
        logger.info("Display initialized")
    
    def stop(self):
        """Stop display manager."""
        if not self._initialized:
            return
        
        logger.info("Stopping display...")
        
        # TODO: Cleanup display in Phase 3
        # - Turn off front-light
        # - Put display in sleep mode
        
        self._initialized = False
        logger.info("Display stopped")
    
    def update(self, sensors_data: Dict, ha_entities: Dict[str, Dict]):
        """
        Update display with new data.
        
        Args:
            sensors_data: Sensor data dictionary
            ha_entities: Home Assistant entities dictionary
        """
        if not self._initialized:
            return
        
        # TODO: Implement display update in Phase 3
        # - Render UI with sensors_data and ha_entities
        # - Convert to 1-bit image
        # - Send to E-Paper display
        # - Handle refresh mode (full/partial)
        
        logger.debug("Display update requested")
