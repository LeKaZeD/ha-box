"""
Status fetcher: Zigbee (ZHA, Zigbee2MQTT).
- ZHA (Zigbee Home Automation): Core integration only, no add-on → detected via entities "zha.*".
- Zigbee2MQTT: add-on (slug contains "zigbee2mqtt", any repo) or entities "zigbee2mqtt.*".
Works with multipan (e.g. ZHA on same stick as Thread).
"""

import logging
from typing import Dict, Any, Optional, List
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)

# Entity domain prefixes that indicate Zigbee integration is loaded
ZIGBEE_ENTITY_PREFIXES = ("zha.", "zigbee2mqtt.")


def _addon_slug_matches_zigbee(slug: str) -> bool:
    """True if this add-on slug indicates a Zigbee stack (Zigbee2MQTT, any repo)."""
    if not slug:
        return False
    return "zigbee2mqtt" in slug.lower()


def fetch_zigbee(ha_api: HomeAssistantAPI, addons_list: Optional[List[Dict[str, Any]]] = None) -> Dict[str, Any]:
    """
    Detect Zigbee: ZHA (Core entities zha.*) or Zigbee2MQTT add-on / entities.

    Args:
        ha_api: API client.
        addons_list: Optional pre-fetched add-ons list (avoids duplicate GET /addons).

    Returns:
        Dict with key: zigbee_ok (bool).
    """
    out: Dict[str, Any] = {"zigbee_ok": False}
    try:
        # 1) Add-ons: any Zigbee add-on started
        if addons_list is None:
            addons_list = ha_api.get_addons_list()
        if isinstance(addons_list, list):
            for addon in addons_list:
                if not isinstance(addon, dict):
                    continue
                slug = addon.get("slug", "")
                state = addon.get("state", "")
                if _addon_slug_matches_zigbee(slug) and state in ("started", "running"):
                    out["zigbee_ok"] = True
                    return out
        # 2) Core: ZHA or Zigbee2MQTT entities present
        states = ha_api.get_all_states()
        if isinstance(states, list):
            for item in states:
                eid = item.get("entity_id") if isinstance(item, dict) else None
                if isinstance(eid, str) and eid.startswith(ZIGBEE_ENTITY_PREFIXES):
                    out["zigbee_ok"] = True
                    return out
    except Exception as e:
        logger.debug("fetch_zigbee failed: %s", e)
    return out
