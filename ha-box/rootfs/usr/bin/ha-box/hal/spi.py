"""SPI bus management."""

import logging
from typing import Optional

logger = logging.getLogger(__name__)

class SPIManager:
    """Manager for SPI bus access."""
    
    def __init__(self, bus: int = 0, device: int = 0, max_speed: int = 1000000):
        """
        Initialize SPI manager.
        
        Args:
            bus: SPI bus number (default: 0 for /dev/spidev0.0)
            device: SPI device number (default: 0)
            max_speed: Maximum SPI speed in Hz (default: 1MHz)
        """
        self.bus = bus
        self.device = device
        self.max_speed = max_speed
        self._spi = None
        self._available = False
        
        try:
            import spidev
            self._spi = spidev.SpiDev()
            self._spi.open(bus, device)
            self._spi.max_speed_hz = max_speed
            self._spi.mode = 0  # SPI mode 0
            self._available = True
            logger.info(f"SPI bus {bus}.{device} initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize SPI bus {bus}.{device}: {e}")
            self._available = False
    
    @property
    def available(self) -> bool:
        """Check if SPI bus is available."""
        return self._available
    
    def write(self, data: list) -> bool:
        """
        Write data to SPI device.
        
        Args:
            data: List of bytes to write
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("SPI bus not available")
            return False
        
        try:
            self._spi.writebytes(data)
            return True
        except Exception as e:
            logger.error(f"Error writing to SPI: {e}")
            return False
    
    def read(self, length: int) -> Optional[list]:
        """
        Read data from SPI device.
        
        Args:
            length: Number of bytes to read
        
        Returns:
            List of bytes or None on error
        """
        if not self._available:
            logger.warning("SPI bus not available")
            return None
        
        try:
            return self._spi.readbytes(length)
        except Exception as e:
            logger.error(f"Error reading from SPI: {e}")
            return None
    
    def transfer(self, data: list) -> Optional[list]:
        """
        Transfer data (write and read simultaneously).
        
        Args:
            data: List of bytes to write
        
        Returns:
            List of bytes read or None on error
        """
        if not self._available:
            logger.warning("SPI bus not available")
            return None
        
        try:
            return self._spi.xfer2(data)
        except Exception as e:
            logger.error(f"Error transferring SPI data: {e}")
            return None
    
    def close(self):
        """Close SPI bus connection."""
        if self._spi:
            try:
                self._spi.close()
                logger.info("SPI bus closed")
            except Exception as e:
                logger.error(f"Error closing SPI bus: {e}")
