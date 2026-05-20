"""
Sensor data handler for ESP32 sensors.

This module handles incoming SENS (BME280) and CASE (TMP102) messages from the
ESP32 and manages sensor data updates to Home Assistant.
"""

import logging
from typing import Dict, Optional
from communication.esp32_comm import Message
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


class SensorHandler:
    """Handler for ESP32 sensor data (BME280 + TMP102 case temperature)."""
    
    def __init__(self, ha_api: Optional[HomeAssistantAPI] = None) -> None:
        """
        Initialize sensor handler.

        Args:
            ha_api: Optional Home Assistant API client for sensor updates.
        """
        self.ha_api = ha_api
        self.latest_data: Optional[Dict[str, float]] = None
        self.latest_case_temp: Optional[float] = None
    
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
    
    def handle_case_message(self, msg: Message) -> None:
        """
        Handle CASE message from ESP32 (TMP102 case temperature).

        Args:
            msg: CASE message with tC key (temperature in Celsius).
        """
        temp_str = msg.get("tC")
        if temp_str:
            try:
                self.latest_case_temp = float(temp_str)
                logger.info("Case temperature: %.2f°C", self.latest_case_temp)
            except ValueError as e:
                logger.error("Failed to parse CASE tC value: %s", e)
        else:
            logger.warning("CASE message missing tC key")

    def update_home_assistant(self) -> bool:
        """
        Update Home Assistant sensors with latest data.

        Pushes BME280 data (SENS) and TMP102 case temperature (CASE) to HA.
        Each sensor type is updated independently; a missing value is silently
        skipped rather than blocking the other.
        
        Returns:
            True if all available updates succeeded, False if any failed.
        """
        if not self.ha_api:
            logger.debug("update_home_assistant() skipped: no ha_api configured")
            return False

        success = True

        if self.latest_data:
            result = self.ha_api.update_esp32_sensors(
                self.latest_data['temperature'],
                self.latest_data['humidity'],
                self.latest_data['pressure']
            )
            success &= result

        if self.latest_case_temp is not None:
            result = self.ha_api.update_case_sensor(self.latest_case_temp)
            success &= result

        if not success:
            logger.warning("update_home_assistant() failed: one or more events could not be fired")

        return success
    
    def clear_latest_data(self) -> None:
        """Clear all latest sensor data after processing."""
        self.latest_data = None
        self.latest_case_temp = None
