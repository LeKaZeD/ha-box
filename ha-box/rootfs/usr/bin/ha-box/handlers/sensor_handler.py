"""
Sensor data handler for ESP32 BME280 sensor.

This module handles incoming SENS messages from the ESP32 and manages
sensor data updates to Home Assistant.
"""

import logging
from typing import Optional, Dict, Any
from communication.esp32_comm import Message
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


class SensorHandler:
    """Handler for ESP32 sensor data (BME280)."""
    
    def __init__(self, ha_api: Optional[HomeAssistantAPI] = None) -> None:
        """
        Initialize sensor handler.
        
        Args:
            ha_api: Optional Home Assistant API client for sensor updates.
        """
        self.ha_api = ha_api
        self.latest_data: Optional[Dict[str, float]] = None
    
    def handle_sens_message(self, msg: Message) -> None:
        """
        Handle SENS message from ESP32.
        
        Args:
            msg: SENS message with temperature, humidity, and pressure.
        """
        try:
            temp_str = msg.get("tC")
            hum_str = msg.get("hum")
            pressure_str = msg.get("pPa")
            
            if temp_str and hum_str and pressure_str:
                temperature = float(temp_str)
                humidity = float(hum_str)
                pressure = float(pressure_str)
                
                logger.info(
                    "Sensor data: T=%.2f°C, H=%.2f%%, P=%.0fPa",
                    temperature, humidity, pressure
                )
                
                # Store latest sensor data for async update
                self.latest_data = {
                    'temperature': temperature,
                    'humidity': humidity,
                    'pressure': pressure
                }
            else:
                logger.warning("SENS message missing data: tC=%s, hum=%s, pPa=%s", 
                             temp_str, hum_str, pressure_str)
        except ValueError as e:
            logger.error("Failed to parse sensor data: %s", e)
    
    def update_home_assistant(self) -> bool:
        """
        Update Home Assistant sensors with latest data.
        
        Returns:
            True if update was successful, False otherwise.
        """
        logger.debug("SensorHandler.update_home_assistant() called: latest_data=%s, ha_api=%s",
                    self.latest_data is not None, self.ha_api is not None)
        
        if not self.latest_data:
            logger.debug("SensorHandler.update_home_assistant() skipped: no latest_data")
            return False
        
        if not self.ha_api:
            logger.debug("SensorHandler.update_home_assistant() skipped: no HA API client")
            return False
        
        logger.debug("Updating HA sensors: T=%.2f°C, H=%.2f%%, P=%.0fPa",
                    self.latest_data['temperature'],
                    self.latest_data['humidity'],
                    self.latest_data['pressure'])
        
        result = self.ha_api.update_esp32_sensors(
            self.latest_data['temperature'],
            self.latest_data['humidity'],
            self.latest_data['pressure']
        )
        
        logger.debug("SensorHandler.update_home_assistant() result: %s", result)
        return result
    
    def clear_latest_data(self) -> None:
        """Clear the latest sensor data after processing."""
        self.latest_data = None
