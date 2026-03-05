"""Home Assistant API client for HA Box."""

import logging
import os
import time
from typing import Dict, Any, List, Optional
import requests

logger = logging.getLogger(__name__)

# Slugs of add-ons that indicate "exposed on internet" when started (DuckDNS, Cloudflare, etc.)
DEFAULT_EXPOSED_ADDON_SLUGS = frozenset({
    "a0d7b954_duckdns",
    "core_duckdns",
    "a0d7b954_cloudflare",
    "core_cloudflare",
})

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

    def get_core_config(self) -> Optional[Dict[str, Any]]:
        """
        Get Home Assistant Core configuration (GET /core/api/config).
        Contains language, time_zone, unit_system, etc.

        Returns:
            Config dict or None if Core unavailable or request failed.
        """
        result = self._request("GET", "/core/api/config")
        return result if isinstance(result, dict) else None

    def get_core_language(self) -> Optional[str]:
        """
        Get the default language from Home Assistant Core config.
        This is the language set in Settings > System > General.

        Returns:
            Language code (e.g. 'en', 'fr', 'fr-FR') or None if unavailable.
        """
        config = self.get_core_config()
        if not config:
            return None
        lang = config.get("language")
        if not lang or not isinstance(lang, str):
            return None
        return lang.strip()

    def request_host_shutdown(self) -> bool:
        """
        Request host shutdown via Supervisor API (POST /host/shutdown).
        Used when ESP32 sends SHUTDOWN_REQUEST (short press on power button).

        Requires hassio_role: manager or admin.

        Returns:
            True if the Supervisor accepted the shutdown request, False otherwise.
        """
        try:
            # POST with longer timeout: Supervisor may take a moment to initiate shutdown
            response = requests.post(
                f"{self.base_url}/host/shutdown",
                headers=self.headers,
                json={},
                timeout=5.0,
            )
            if 200 <= response.status_code < 300:
                logger.info("Host shutdown request accepted by Supervisor (status=%d)", response.status_code)
                return True
            logger.warning(
                "Host shutdown request rejected: status=%d, body=%s",
                response.status_code,
                response.text[:200] if response.text else "(empty)",
            )
            return False
        except requests.exceptions.ConnectionError as e:
            # Connection may drop if host starts shutting down immediately
            logger.info("Host shutdown request sent; connection closed (shutdown may have started): %s", e)
            return True
        except requests.exceptions.Timeout:
            logger.warning("Host shutdown request timed out (shutdown may still be in progress)")
            return True  # Optimistic: timeout might mean shutdown started
        except Exception as e:
            logger.error("Host shutdown request failed: %s", e, exc_info=True)
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

    def get_all_states(self) -> Optional[List[Dict[str, Any]]]:
        """
        Get all entity states from Home Assistant Core (GET /core/api/states).
        Used to detect integrations by entity domain (e.g. zha.*, matter.*).

        Returns:
            List of state dicts (each with entity_id, state, attributes, ...) or None on error.
        """
        if not self._check_core_available():
            return None
        result = self._request("GET", "/core/api/states")
        if result is None:
            return None
        if isinstance(result, list):
            return result
        logger.debug("get_all_states: unexpected response type %s", type(result))
        return None
    
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

    # -------------------------------------------------------------------------
    # Supervisor / system status (for STATUS message to ESP32)
    # -------------------------------------------------------------------------

    def get_supervisor_info(self) -> Optional[Dict[str, Any]]:
        """
        Get Supervisor info from the Supervisor API (no Core dependency).

        Returns:
            Response dict (e.g. result, version, data) or None on error.
        """
        result = self._request("GET", "/supervisor/info")
        if result is not None:
            logger.debug("get_supervisor_info: %s", list(result.keys()) if isinstance(result, dict) else result)
        return result

    def get_network_info(self) -> Optional[Dict[str, Any]]:
        """
        Get network interfaces from the Supervisor API.

        Returns:
            Response dict containing network/interfaces data, or None on error.
            Typical structure: interfaces list with type (ethernet/wireless), connected, primary.
        """
        result = self._request("GET", "/network/info")
        if result is not None:
            logger.debug("get_network_info: %s", list(result.keys()) if isinstance(result, dict) else result)
        return result

    def get_addons_list(self) -> Optional[List[Dict[str, Any]]]:
        """
        Get list of add-ons from the Supervisor API.
        Response shape may be {"data": {"addons": [...]}} or {"addons": [...]}; returns the list.

        Returns:
            List of add-on dicts (slug, state, name, version, ...) or None on error.
        """
        result = self._request("GET", "/addons")
        if result is None:
            return None
        if isinstance(result, dict):
            if "data" in result and isinstance(result["data"], dict) and "addons" in result["data"]:
                return result["data"]["addons"]
            if "addons" in result and isinstance(result["addons"], list):
                return result["addons"]
            if "data" in result and isinstance(result["data"], list):
                return result["data"]
        if isinstance(result, list):
            return result
        logger.warning("get_addons_list: unexpected response shape %s", type(result))
        return None

    def get_addon_info(self, slug: str) -> Optional[Dict[str, Any]]:
        """
        Get add-on info (including options) from Supervisor API.
        GET /addons/<slug>/info returns options with e.g. DuckDNS "domains", Cloudflare "external_hostname".
        Supervisor wraps the payload in {"result":"ok","data":{...}}; we return the inner "data" dict.

        Returns:
            Add-on info dict (options, state, ...) or None on error.
        """
        if not slug:
            return None
        result = self._request("GET", f"/addons/{slug}/info")
        if not isinstance(result, dict):
            return None
        # Supervisor API wraps addon info in result.data
        if "data" in result and isinstance(result["data"], dict):
            return result["data"]
        return result

    def get_system_status(
        self,
        exposed_addon_slugs: Optional[frozenset] = None,
    ) -> Dict[str, bool]:
        """
        Aggregate system status for the STATUS message to ESP32.
        Does not require Core to be available for Supervisor/network/addons.

        Args:
            exposed_addon_slugs: Set of add-on slugs that mean "exposed" when started.
                                If None, uses DEFAULT_EXPOSED_ADDON_SLUGS.

        Returns:
            Dict with keys: core_ok, supervisor_ok, network_ok, lan_ok, wifi_ok, exposed_ok (all bool).
        """
        slugs = exposed_addon_slugs if exposed_addon_slugs is not None else DEFAULT_EXPOSED_ADDON_SLUGS
        out = {
            "core_ok": False,
            "supervisor_ok": False,
            "network_ok": False,
            "lan_ok": False,
            "wifi_ok": False,
            "exposed_ok": False,
        }

        # Core: reuse existing check (may use cache)
        out["core_ok"] = self._check_core_available()

        # Supervisor
        sup = self.get_supervisor_info()
        if sup is not None and isinstance(sup, dict):
            result = sup.get("result")
            out["supervisor_ok"] = result == "ok"

        # Network: at least one interface connected; derive lan/wifi from type
        net = self.get_network_info()
        if net is not None and isinstance(net, dict):
            interfaces = net.get("interfaces")
            if interfaces is None and isinstance(net.get("data"), dict):
                interfaces = net["data"].get("interfaces")
            if interfaces is None and isinstance(net.get("data"), list):
                interfaces = net["data"]
            if isinstance(interfaces, list):
                for iface in interfaces:
                    if not isinstance(iface, dict):
                        continue
                    connected = iface.get("connected") or iface.get("state") == "connected"
                    if not connected:
                        continue
                    out["network_ok"] = True
                    itype = (iface.get("type") or "").lower()
                    if itype == "ethernet":
                        out["lan_ok"] = True
                    elif itype == "wireless":
                        out["wifi_ok"] = True
                if out["network_ok"] and not out["lan_ok"] and not out["wifi_ok"]:
                    out["lan_ok"] = True

        # Exposed: any of the configured add-ons is started
        addons = self.get_addons_list()
        if isinstance(addons, list):
            for addon in addons:
                if not isinstance(addon, dict):
                    continue
                if addon.get("slug") in slugs and (addon.get("state") == "started" or addon.get("state") == "running"):
                    out["exposed_ok"] = True
                    break
        else:
            logger.debug("get_system_status: addons list not available (403 or other)")

        logger.debug(
            "get_system_status: core=%s sup=%s net=%s lan=%s wifi=%s ext=%s",
            out["core_ok"], out["supervisor_ok"], out["network_ok"], out["lan_ok"], out["wifi_ok"], out["exposed_ok"],
        )
        return out

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
