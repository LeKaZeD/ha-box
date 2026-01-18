"""Home Assistant API client."""

import logging
import os
from typing import Dict, List, Optional

logger = logging.getLogger(__name__)

class HAClient:
    """Client for Home Assistant Supervisor API."""
    
    def __init__(self):
        """Initialize Home Assistant client."""
        self.base_url = os.environ.get("SUPERVISOR_URL", "http://supervisor")
        self.token = os.environ.get("SUPERVISOR_TOKEN")
        
        if not self.token:
            logger.warning("SUPERVISOR_TOKEN not found, API calls may fail")
        
        self._session = None
        self._available = False
        
        try:
            import requests
            self._session = requests.Session()
            if self.token:
                self._session.headers.update({
                    "Authorization": f"Bearer {self.token}",
                    "Content-Type": "application/json"
                })
            self._available = True
            logger.info("Home Assistant client initialized")
        except Exception as e:
            logger.error(f"Failed to initialize Home Assistant client: {e}")
            self._available = False
    
    @property
    def available(self) -> bool:
        """Check if Home Assistant API is available."""
        return self._available
    
    def get_entities(self, entity_ids: List[str]) -> Dict[str, Dict]:
        """
        Get state of multiple entities.
        
        Args:
            entity_ids: List of entity IDs to fetch
        
        Returns:
            Dictionary mapping entity_id to state data
        """
        if not self._available:
            logger.warning("Home Assistant API not available")
            return {}
        
        entities = {}
        
        for entity_id in entity_ids:
            try:
                state = self.get_entity_state(entity_id)
                if state:
                    entities[entity_id] = state
            except Exception as e:
                logger.error(f"Error fetching entity {entity_id}: {e}")
        
        return entities
    
    def get_entity_state(self, entity_id: str) -> Optional[Dict]:
        """
        Get state of a single entity.
        
        Args:
            entity_id: Entity ID (e.g., "sensor.temperature_salon")
        
        Returns:
            Entity state dictionary or None on error
        """
        if not self._available:
            logger.warning("Home Assistant API not available")
            return None
        
        try:
            url = f"{self.base_url}/core/api/states/{entity_id}"
            response = self._session.get(url, timeout=5)
            response.raise_for_status()
            return response.json()
        except Exception as e:
            logger.error(f"Error fetching entity state for {entity_id}: {e}")
            return None
    
    def call_service(self, domain: str, service: str, service_data: Optional[Dict] = None) -> bool:
        """
        Call a Home Assistant service.
        
        Args:
            domain: Service domain (e.g., "light")
            service: Service name (e.g., "turn_on")
            service_data: Optional service data
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("Home Assistant API not available")
            return False
        
        try:
            url = f"{self.base_url}/core/api/services/{domain}/{service}"
            data = service_data or {}
            response = self._session.post(url, json=data, timeout=5)
            response.raise_for_status()
            return True
        except Exception as e:
            logger.error(f"Error calling service {domain}.{service}: {e}")
            return False
    
    def fire_event(self, event_type: str, event_data: Optional[Dict] = None) -> bool:
        """
        Fire a Home Assistant event.
        
        Args:
            event_type: Event type (e.g., "nfc_tag_scanned")
            event_data: Optional event data
        
        Returns:
            True if successful, False otherwise
        """
        if not self._available:
            logger.warning("Home Assistant API not available")
            return False
        
        try:
            url = f"{self.base_url}/core/api/events/{event_type}"
            data = event_data or {}
            response = self._session.post(url, json=data, timeout=5)
            response.raise_for_status()
            return True
        except Exception as e:
            logger.error(f"Error firing event {event_type}: {e}")
            return False
