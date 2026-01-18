"""Control manager for actuators (fan, LED)."""

import logging
from typing import Optional

from ha_box.config import Config
from ha_box.hal.gpio import GPIOManager
from ha_box.sensors.manager import SensorsManager

logger = logging.getLogger(__name__)

class ControlManager:
    """Manager for control devices (fan, LED)."""
    
    def __init__(self, config: Config, gpio_manager: GPIOManager, sensors_manager: SensorsManager):
        """
        Initialize control manager.
        
        Args:
            config: Configuration object
            gpio_manager: GPIO manager instance
            sensors_manager: Sensors manager instance (for auto control)
        """
        self.config = config
        self.gpio_manager = gpio_manager
        self.sensors_manager = sensors_manager
        self._running = False
        
        # Initialize control devices (to be implemented in Phase 4)
        self._fan = None
        self._led = None
    
    def start(self):
        """Start control manager."""
        logger.info("Starting control manager...")
        
        # TODO: Initialize control devices in Phase 4
        # - Setup fan PWM if enabled
        # - Setup LED strip if enabled
        
        self._running = True
        logger.info("Control manager started")
    
    def stop(self):
        """Stop control manager."""
        if not self._running:
            return
        
        logger.info("Stopping control manager...")
        
        # TODO: Cleanup control devices in Phase 4
        # - Turn off fan
        # - Turn off LED
        
        self._running = False
        logger.info("Control manager stopped")
    
    def update_fan(self, temperature: Optional[float]):
        """
        Update fan speed based on temperature.
        
        Args:
            temperature: Current temperature in Celsius
        """
        if not self.config.control.fan.enabled or not self._running:
            return
        
        # TODO: Implement fan control in Phase 4
        # - Calculate fan speed based on temperature
        # - Apply PWM control
        
        logger.debug(f"Fan update requested (temp: {temperature}°C)")
