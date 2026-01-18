#!/usr/bin/env python3
"""
HA Box - Main application entry point.

This is the main entry point for the HA Box add-on.
"""

import logging
import sys
from pathlib import Path

# Add the ha-box package to the path
sys.path.insert(0, str(Path(__file__).parent))

from ha_box.config import Config
from ha_box.logger import setup_logging
from ha_box.ha.client import HAClient
from ha_box.hal.i2c import I2CManager
from ha_box.hal.spi import SPIManager
from ha_box.hal.gpio import GPIOManager
from ha_box.sensors.manager import SensorsManager
from ha_box.display.manager import DisplayManager
from ha_box.control.manager import ControlManager

# Setup logging
logger = setup_logging()

def main() -> None:
    """Main application loop."""
    logger.info("HA Box starting...")
    
    try:
        # Load configuration
        from ha_box.config import load_options
        options = load_options()
        config = Config.from_options(options)
        logger.info("Configuration loaded")
        
        # Initialize Hardware Abstraction Layer
        logger.info("Initializing hardware abstraction layer...")
        i2c_manager = I2CManager()
        spi_manager = SPIManager()
        gpio_manager = GPIOManager()
        
        # Initialize Home Assistant client
        logger.info("Connecting to Home Assistant...")
        ha_client = HAClient()
        
        # Initialize managers
        sensors_manager = SensorsManager(
            config=config,
            i2c_manager=i2c_manager
        )
        
        display_manager = DisplayManager(
            config=config,
            spi_manager=spi_manager,
            gpio_manager=gpio_manager
        )
        
        control_manager = ControlManager(
            config=config,
            gpio_manager=gpio_manager,
            sensors_manager=sensors_manager
        )
        
        # Start managers
        logger.info("Starting managers...")
        sensors_manager.start()
        display_manager.start()
        control_manager.start()
        
        # Main loop
        logger.info("HA Box running...")
        try:
            while True:
                # Update display with sensor data and HA entities
                sensors_data = sensors_manager.get_data()
                ha_entities = ha_client.get_entities(config.home_assistant.entities)
                
                display_manager.update(sensors_data, ha_entities)
                
                # Sleep for update interval
                import time
                time.sleep(config.home_assistant.update_interval)
                
        except KeyboardInterrupt:
            logger.info("Shutdown requested")
        finally:
            # Cleanup
            logger.info("Stopping managers...")
            control_manager.stop()
            display_manager.stop()
            sensors_manager.stop()
            logger.info("HA Box stopped")
            
    except Exception as e:
        logger.exception(f"Fatal error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
