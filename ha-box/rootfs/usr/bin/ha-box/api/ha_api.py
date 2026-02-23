"""Home Assistant API client for HA Box."""

import logging
import os
import time
from typing import Dict, Any, Optional
import requests

logger = logging.getLogger(__name__)

class HomeAssistantAPI:
    """Client for Home Assistant Supervisor API."""
    
    def __init__(self):
        """Initialize Home Assistant API client."""
        self.supervisor_token = os.environ.get("SUPERVISOR_TOKEN", "")
        
        # Use SUPERVISOR_URL if available, otherwise fallback to IP
        supervisor_url = os.environ.get("SUPERVISOR_URL")
        if supervisor_url:
            self.base_url = supervisor_url
            logger.debug("Using SUPERVISOR_URL from environment: %s", supervisor_url)
        else:
            # Fallback to fixed IP (standard Supervisor IP in HAOS)
            self.base_url = "http://172.30.32.2"
            logger.debug("Using Supervisor IP fallback: %s", self.base_url)
        
        self.headers = {
            "Authorization": f"Bearer {self.supervisor_token}",
            "Content-Type": "application/json",
        }
        self.timeout = 3.0  # Reduced from 10 to 3 seconds
        self._core_available = False
        self._last_check = 0
        self._check_interval = 30.0  # Check Core availability every 30 seconds
        
        logger.info("Home Assistant API client initialized (base_url=%s)", self.base_url)
    
    def _check_core_available(self) -> bool:
        """
        Check if Home Assistant Core is available.
        
        Returns:
            True if Core is available, False otherwise.
        """
        now = time.time()
        
        # Cache check result for a short time to avoid excessive API calls
        if now - self._last_check < self._check_interval and self._core_available:
            logger.debug("_check_core_available() using cached result: %s", self._core_available)
            return self._core_available
        
        logger.debug("_check_core_available() checking Core availability...")
        self._last_check = now
        
        try:
            url = f"{self.base_url}/core/api/"
            logger.debug("Checking Core at: %s", url)
            
            response = requests.get(
                url,
                headers=self.headers,
                timeout=2.0,
            )
            
            logger.debug("Core check response: status_code=%d", response.status_code)
            
            self._core_available = response.status_code == 200
            if self._core_available:
                logger.info("Home Assistant Core is available")
            else:
                logger.warning("Home Assistant Core returned status %d (expected 200)", response.status_code)
                if response.text:
                    logger.debug("Core check response body: %s", response.text[:200])
            return self._core_available
            
        except requests.exceptions.ConnectionError as e:
            logger.warning("Home Assistant Core connection error: %s", e)
            if self._core_available:
                logger.warning("  Core was previously available, now unreachable")
            self._core_available = False
            return False
        except requests.exceptions.Timeout as e:
            logger.warning("Home Assistant Core timeout: %s", e)
            self._core_available = False
            return False
        except requests.exceptions.RequestException as e:
            logger.warning("Home Assistant Core request error: %s", e)
            if self._core_available:
                logger.warning("  Core was previously available, now error occurred")
            self._core_available = False
            return False
        except Exception as e:
            logger.error("Home Assistant Core check unexpected error: %s", e, exc_info=True)
            self._core_available = False
            return False
    
    def _request(
        self, method: str, endpoint: str, json: Optional[Dict[str, Any]] = None
    ) -> Optional[Dict[str, Any]]:
        """
        Make a request to Home Assistant Supervisor API.
        
        Args:
            method: HTTP method (GET, POST, etc.)
            endpoint: API endpoint (e.g., "/core/api/states/sensor.temperature")
            json: Optional JSON payload
            
        Returns:
            Response JSON or None on error
        """
        url = f"{self.base_url}{endpoint}"
        logger.debug("Making API request: %s %s", method, url)
        
        try:
            response = requests.request(
                method=method,
                url=url,
                headers=self.headers,
                json=json,
                timeout=self.timeout,
            )
            
            logger.debug("API response: status_code=%d, headers=%s", 
                        response.status_code, dict(response.headers))
            
            # Log response body for debugging (truncated if too long)
            if response.text:
                body_preview = response.text[:200] if len(response.text) > 200 else response.text
                logger.debug("API response body (preview): %s", body_preview)
            
            response.raise_for_status()
            
            result = response.json() if response.text else {}
            logger.debug("API request successful: %s %s", method, endpoint)
            return result
            
        except requests.exceptions.HTTPError as e:
            logger.error("API HTTP error: %s %s - Status: %d, Response: %s", 
                       method, endpoint, response.status_code, response.text[:200] if response.text else "No body")
            return None
        except requests.exceptions.ConnectionError as e:
            logger.error("API connection error: %s %s - %s", method, endpoint, e)
            return None
        except requests.exceptions.Timeout as e:
            logger.error("API timeout: %s %s - %s", method, endpoint, e)
            return None
        except requests.exceptions.RequestException as e:
            logger.error("API request exception: %s %s - %s", method, endpoint, e)
            return None
        except Exception as e:
            logger.error("API unexpected error: %s %s - %s", method, endpoint, e, exc_info=True)
            return None
    
    def update_sensor(
        self, entity_id: str, state: str, attributes: Optional[Dict[str, Any]] = None
    ) -> bool:
        """
        Update a sensor state in Home Assistant.
        
        Args:
            entity_id: Sensor entity ID (e.g., "sensor.esp32_temperature")
            state: Sensor state value
            attributes: Optional sensor attributes
            
        Returns:
            True if successful, False otherwise
        """
        # Check if Core is available before trying to update
        if not self._check_core_available():
            logger.debug("Skipping sensor update, Core not available")
            return False
        
        endpoint = f"/core/api/states/{entity_id}"
        payload = {
            "state": state,
            "attributes": attributes or {},
        }
        
        result = self._request("POST", endpoint, json=payload)
        if result:
            logger.debug("Sensor %s updated: %s", entity_id, state)
            return True
        return False
    
    def get_state(self, entity_id: str) -> Optional[Dict[str, Any]]:
        """
        Get the state of an entity from Home Assistant.
        
        Args:
            entity_id: Entity ID (e.g., "weather.home")
            
        Returns:
            Entity state dict or None on error
        """
        logger.debug("get_state() called for entity: %s", entity_id)
        
        if not self._check_core_available():
            logger.warning("get_state() skipped: Home Assistant Core not available")
            return None
        
        endpoint = f"/core/api/states/{entity_id}"
        logger.debug("Fetching state from endpoint: %s", endpoint)
        
        result = self._request("GET", endpoint)
        
        if result:
            logger.debug("get_state() successful for %s: state=%s", entity_id, result.get("state"))
        else:
            logger.warning("get_state() failed for %s: API returned None", entity_id)
        
        return result
    
    def get_weather(self, entity_id: str = "weather.home") -> Optional[Dict[str, Any]]:
        """
        Get weather data from Home Assistant.
        
        Args:
            entity_id: Weather entity ID
            
        Returns:
            Dict with 'condition' (str) and 'temperature' (float) or None
        """
        logger.debug("get_weather() called for entity: %s", entity_id)
        
        state = self.get_state(entity_id)
        if not state:
            logger.warning("get_weather() failed: Could not get state for entity %s", entity_id)
            logger.debug("  This could mean:")
            logger.debug("  - Entity does not exist in Home Assistant")
            logger.debug("  - Home Assistant Core is not ready")
            logger.debug("  - API request failed (check previous logs)")
            return None
        
        logger.debug("get_weather() state received: %s", state)
        
        try:
            condition = state.get("state", "unknown")
            attributes = state.get("attributes", {})
            temperature = attributes.get("temperature")
            
            logger.debug("get_weather() parsed: condition=%s, attributes=%s, temperature=%s", 
                        condition, list(attributes.keys()) if attributes else "None", temperature)
            
            if temperature is None:
                logger.warning("get_weather() failed: Weather entity %s has no temperature attribute", entity_id)
                logger.debug("  Available attributes: %s", list(attributes.keys()) if attributes else "None")
                return None
            
            logger.info("get_weather() successful: condition=%s, temp=%.1f°C", condition, temperature)
            
            return {
                "condition": condition,
                "temperature": float(temperature),
            }
        except (KeyError, ValueError, TypeError) as e:
            logger.error("get_weather() failed to parse data: %s", e, exc_info=True)
            logger.debug("  State data: %s", state)
            return None
    
    def update_esp32_sensors(
        self, temperature: float, humidity: float, pressure: float
    ) -> bool:
        """
        Update ESP32 sensor data in Home Assistant.
        
        Args:
            temperature: Temperature in Celsius
            humidity: Humidity in %
            pressure: Pressure in Pa
            
        Returns:
            True if all updates successful
        """
        logger.debug("Updating sensors: T=%.2f°C, H=%.2f%%, P=%.0fPa", 
                    temperature, humidity, pressure)
        
        # Check if Core is available before trying to update
        if not self._check_core_available():
            logger.debug("Skipping ESP32 sensor update, Home Assistant Core not ready")
            return False
        
        success = True
        
        # Temperature sensor
        success &= self.update_sensor(
            "sensor.ha_box_temperature",
            f"{temperature:.2f}",
            {
                "unit_of_measurement": "°C",
                "device_class": "temperature",
                "friendly_name": "HA Box Temperature",
                "state_class": "measurement",
            },
        )
        
        # Humidity sensor
        success &= self.update_sensor(
            "sensor.ha_box_humidity",
            f"{humidity:.2f}",
            {
                "unit_of_measurement": "%",
                "device_class": "humidity",
                "friendly_name": "HA Box Humidity",
                "state_class": "measurement",
            },
        )
        
        # Pressure sensor (convert Pa to hPa)
        pressure_hpa = pressure / 100.0
        success &= self.update_sensor(
            "sensor.ha_box_pressure",
            f"{pressure_hpa:.2f}",
            {
                "unit_of_measurement": "hPa",
                "device_class": "pressure",
                "friendly_name": "HA Box Pressure",
                "state_class": "measurement",
            },
        )
        
        if success:
            logger.info("ESP32 sensors updated successfully")
        else:
            logger.debug("Some ESP32 sensor updates failed")
        
        return success

    def call_light_turn_off_all(self) -> bool:
        """
        Call Home Assistant service light.turn_off with entity_id "all"
        to turn off all lights in the house.

        Returns:
            True if the service call succeeded, False otherwise.
        """
        if not self._check_core_available():
            logger.warning("call_light_turn_off_all skipped: Core not available")
            return False
        endpoint = "/core/api/services/light/turn_off"
        payload = {"entity_id": "all"}
        result = self._request("POST", endpoint, json=payload)
        if result is not None:
            logger.info("light.turn_off (all) called successfully")
            return True
        logger.warning("call_light_turn_off_all failed")
        return False
