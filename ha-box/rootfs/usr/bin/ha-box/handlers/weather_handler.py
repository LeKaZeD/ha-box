"""
Weather data handler for ESP32.

This module handles fetching weather data from Home Assistant and
sending it to the ESP32.
"""

import logging
import threading
import time
from typing import Optional, Dict, Any, Callable
from communication.esp32_comm import ESP32Comm, KV
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)

# Weather condition mapping (HA state → numeric code)
WEATHER_MAP = {
    "clear-night": 1,
    "cloudy": 2,
    "fog": 3,
    "hail": 4,
    "lightning": 5,
    "lightning-rainy": 6,
    "partlycloudy": 7,
    "pouring": 8,
    "rainy": 9,
    "snowy": 10,
    "snowy-rainy": 11,
    "sunny": 12,
    "windy": 13,
    "windy-variant": 14,
    "exceptional": 15,
}


class WeatherHandler:
    """Handler for weather data updates to ESP32."""
    
    def __init__(
        self,
        esp32_comm: Optional[ESP32Comm] = None,
        ha_api: Optional[HomeAssistantAPI] = None,
        weather_entity: str = "weather.home",
        update_interval: float = 300.0
    ) -> None:
        """
        Initialize weather handler.
        
        Args:
            esp32_comm: ESP32 communication handler.
            ha_api: Home Assistant API client.
            weather_entity: Weather entity ID in Home Assistant.
            update_interval: Update interval in seconds (default: 5 minutes).
        """
        self.esp32_comm = esp32_comm
        self.ha_api = ha_api
        self.weather_entity = weather_entity
        self.update_interval = update_interval
        self.last_update = 0.0
        self.timer: Optional[threading.Timer] = None
        self._is_connected_callback: Optional[Callable[[], bool]] = None
    
    def should_update(self, current_time: float) -> bool:
        """
        Check if weather should be updated.
        
        Args:
            current_time: Current timestamp.
            
        Returns:
            True if update is needed, False otherwise.
        """
        time_since_update = current_time - self.last_update
        should = time_since_update >= self.update_interval
        logger.debug("Weather should_update: time_since=%.1fs, interval=%.1fs, should=%s", 
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
        Force immediate weather update (for initialization).
        
        Returns:
            True if update was successful, False otherwise.
        """
        return self._send_weather()
    
    def start_periodic_updates(self) -> None:
        """Start periodic weather updates using threading.Timer."""
        logger.info("Starting periodic weather updates (interval=%.1fs)", self.update_interval)
        self._schedule_next()
    
    def stop_periodic_updates(self) -> None:
        """Stop periodic weather updates."""
        if self.timer:
            logger.info("Stopping periodic weather updates")
            self.timer.cancel()
            self.timer = None
    
    def _schedule_next(self) -> None:
        """Schedule next weather update."""
        if self.timer:
            self.timer.cancel()
        
        self.timer = threading.Timer(self.update_interval, self._periodic_update)
        self.timer.start()
        logger.debug("Weather update scheduled in %.1f seconds", self.update_interval)
    
    def _periodic_update(self) -> None:
        """
        Periodic update callback (called by timer).
        Checks connection before sending.
        """
        # Check if still connected
        if self._is_connected_callback and not self._is_connected_callback():
            logger.debug("Weather periodic update skipped: ESP32 not connected")
            # Don't schedule next update if disconnected
            return
        
        # Send weather update
        self._send_weather()
        
        # Schedule next update
        self._schedule_next()
    
    def _send_weather(self) -> bool:
        """
        Fetch weather from Home Assistant and send to ESP32.
        
        Returns:
            True if update was successful, False otherwise.
        """
        logger.debug("WeatherHandler._send_weather() called: esp32_comm=%s, ha_api=%s", 
                    self.esp32_comm is not None, self.ha_api is not None)
        
        if not self.esp32_comm:
            logger.warning("WeatherHandler._send_weather() skipped: esp32_comm is None")
            return False
        
        if not self.ha_api:
            logger.warning("WeatherHandler._send_weather() skipped: ha_api is None")
            return False
        
        try:
            logger.debug("Fetching weather from entity: %s", self.weather_entity)
            weather = self.ha_api.get_weather(self.weather_entity)
            
            if weather:
                condition = weather.get("condition", "unknown")
                temp_out = weather.get("temperature", 0)
                
                logger.debug("Weather data received: condition=%s, temp=%.1f°C", condition, temp_out)
                
                # Map condition to numeric code
                weather_code = WEATHER_MAP.get(condition, 0)
                
                if weather_code == 0:
                    logger.warning("Unknown weather condition '%s', using code 0", condition)
                
                # Convert temperature to x10 format
                temp_out_x10 = int(temp_out * 10)
                
                logger.info("Sending weather to ESP32: code=%d (%s), temp=%.1f°C (tOut=%d)", 
                          weather_code, condition, temp_out, temp_out_x10)
                
                msg_id = self.esp32_comm.send_command("WEATHER", [
                    KV("code", str(weather_code)),
                    KV("tOut", str(temp_out_x10)),
                ])
                
                logger.debug("Weather command sent with msg_id=%d", msg_id)
                
                self.last_update = time.time()
                return msg_id > 0
            else:
                logger.debug("No weather data available from entity: %s", self.weather_entity)
                return False
        except Exception as e:
            logger.error("Failed to send weather update: %s", e, exc_info=True)
            return False
    
    def update(self, current_time: float) -> bool:
        """
        Update weather (legacy method, kept for compatibility).
        
        Args:
            current_time: Current timestamp.
            
        Returns:
            True if update was successful, False otherwise.
        """
        if not self.should_update(current_time):
            return False
        return self._send_weather()