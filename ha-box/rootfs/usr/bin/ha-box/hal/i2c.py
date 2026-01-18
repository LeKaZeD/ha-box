"""I2C bus management."""

import logging
from typing import Optional

logger = logging.getLogger(__name__)

class I2CManager:
    """Manager for I2C bus access."""
    
    def __init__(self, bus: int = 1):
        """
        Initialize I2C manager.
        
        Args:
            bus: I2C bus number (default: 1 for /dev/i2c-1)
        """
        self.bus = bus
        self._smbus = None
        self._available = False
        
        try:
            import smbus2
            self._smbus = smbus2.SMBus(bus)
            self._available = True
            logger.info(f"I2C bus {bus} initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize I2C bus {bus}: {e}")
            self._available = False
    
    @property
    def available(self) -> bool:
        """Check if I2C bus is available."""
        return self._available
    
    def scan(self) -> list:
        """
        Scan I2C bus for devices.
        
        Returns:
            List of detected I2C addresses (hex strings)
        """
        if not self._available:
            logger.warning("I2C bus not available")
            return []
        
        devices = []
        try:
            for address in range(0x08, 0x78):
                try:
                    self._smbus.read_byte(address)
                    devices.append(hex(address))
                    logger.debug(f"Found I2C device at {hex(address)}")
                except Exception:
                    pass
        except Exception as e:
            logger.error(f"Error scanning I2C bus: {e}")
        
        return devices
    
    def read_byte(self, address: int) -> Optional[int]:
        """
        Read a single byte from I2C device.
        
        Args:
            address: I2C device address
        
        Returns:
            Byte value or None on error
        """
        if not self._available:
            logger.warning("I2C bus not available")
            return None
        
        try:
            return self._smbus.read_byte(address)
        except Exception as e:
            logger.error(f"Error reading byte from {hex(address)}: {e}")
            return None
    
    def write_byte(self, address: int, value: int) -> bool:
        """
        Write a single byte to I2C device.
        
        Args:
            address: I2C device address
            value: Byte value to write
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("I2C bus not available")
            return False
        
        try:
            self._smbus.write_byte(address, value)
            return True
        except Exception as e:
            logger.error(f"Error writing byte to {hex(address)}: {e}")
            return False
    
    def read_i2c_block_data(self, address: int, register: int, length: int) -> Optional[list]:
        """
        Read a block of data from I2C device.
        
        Args:
            address: I2C device address
            register: Register address
            length: Number of bytes to read
        
        Returns:
            List of bytes or None on error
        """
        if not self._available:
            logger.warning("I2C bus not available")
            return None
        
        try:
            return self._smbus.read_i2c_block_data(address, register, length)
        except Exception as e:
            logger.error(f"Error reading block from {hex(address)}: {e}")
            return None
    
    def write_i2c_block_data(self, address: int, register: int, data: list) -> bool:
        """
        Write a block of data to I2C device.
        
        Args:
            address: I2C device address
            register: Register address
            data: List of bytes to write
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("I2C bus not available")
            return False
        
        try:
            self._smbus.write_i2c_block_data(address, register, data)
            return True
        except Exception as e:
            logger.error(f"Error writing block to {hex(address)}: {e}")
            return False
    
    def close(self):
        """Close I2C bus connection."""
        if self._smbus:
            try:
                self._smbus.close()
                logger.info("I2C bus closed")
            except Exception as e:
                logger.error(f"Error closing I2C bus: {e}")
