"""GPIO management."""

import logging
from typing import Optional

logger = logging.getLogger(__name__)

class GPIOManager:
    """Manager for GPIO access."""
    
    def __init__(self):
        """Initialize GPIO manager."""
        self._gpio = None
        self._available = False
        
        try:
            import RPi.GPIO as GPIO
            self._gpio = GPIO
            GPIO.setmode(GPIO.BCM)  # Use BCM pin numbering
            GPIO.setwarnings(False)  # Disable warnings
            self._available = True
            logger.info("GPIO initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize GPIO: {e}")
            self._available = False
    
    @property
    def available(self) -> bool:
        """Check if GPIO is available."""
        return self._available
    
    def setup_output(self, pin: int) -> bool:
        """
        Setup GPIO pin as output.
        
        Args:
            pin: GPIO pin number (BCM)
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("GPIO not available")
            return False
        
        try:
            self._gpio.setup(pin, self._gpio.OUT)
            return True
        except Exception as e:
            logger.error(f"Error setting up output pin {pin}: {e}")
            return False
    
    def setup_input(self, pin: int, pull_up_down: Optional[int] = None) -> bool:
        """
        Setup GPIO pin as input.
        
        Args:
            pin: GPIO pin number (BCM)
            pull_up_down: Pull up/down mode (GPIO.PUD_UP, GPIO.PUD_DOWN, or None)
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("GPIO not available")
            return False
        
        try:
            if pull_up_down is not None:
                self._gpio.setup(pin, self._gpio.IN, pull_up_down=pull_up_down)
            else:
                self._gpio.setup(pin, self._gpio.IN)
            return True
        except Exception as e:
            logger.error(f"Error setting up input pin {pin}: {e}")
            return False
    
    def output(self, pin: int, value: bool) -> bool:
        """
        Set GPIO pin output value.
        
        Args:
            pin: GPIO pin number (BCM)
            value: True for HIGH, False for LOW
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("GPIO not available")
            return False
        
        try:
            self._gpio.output(pin, self._gpio.HIGH if value else self._gpio.LOW)
            return True
        except Exception as e:
            logger.error(f"Error setting output pin {pin}: {e}")
            return False
    
    def input(self, pin: int) -> Optional[bool]:
        """
        Read GPIO pin input value.
        
        Args:
            pin: GPIO pin number (BCM)
        
        Returns:
            True for HIGH, False for LOW, or None on error
        """
        if not self._available:
            logger.warning("GPIO not available")
            return None
        
        try:
            return self._gpio.input(pin) == self._gpio.HIGH
        except Exception as e:
            logger.error(f"Error reading input pin {pin}: {e}")
            return None
    
    def cleanup(self):
        """Cleanup GPIO resources."""
        if self._gpio:
            try:
                self._gpio.cleanup()
                logger.info("GPIO cleaned up")
            except Exception as e:
                logger.error(f"Error cleaning up GPIO: {e}")
