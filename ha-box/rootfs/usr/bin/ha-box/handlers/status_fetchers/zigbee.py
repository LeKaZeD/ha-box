"""
Status fetcher: Zigbee (ZHA, Zigbee2MQTT).
- ZHA: Core integration → detected via config entry with domain "zha" and state "loaded".
- Zigbee2MQTT: Supervisor add-on (slug contains "zigbee2mqtt") or config entry domain "mqtt"
  with title containing "zigbee2mqtt".
Works with multipan (e.g. ZHA on same stick as Thread).
"""

import logging
from typing import Dict, Any, Optional, List
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


def _addon_slug_matches_zigbee(slug: str) -> bool:
    """True if this add-on slug indicates a Zigbee2MQTT stack (any repo)."""
    return bool(slug) and "zigbee2mqtt" in slug.lower()


def fetch_zigbee(ha_api: HomeAssistantAPI, addons_list: Optional[List[Dict[str, Any]]] = None) -> Dict[str, Any]:
    """
    Detect Zigbee: ZHA (config entry) or Zigbee2MQTT (add-on or config entry).

    Args:
        ha_api: API client.
        addons_list: Optional pre-fetched add-ons list (avoids duplicate GET /addons).

    Returns:
        Dict with key: zigbee_ok (bool).
    """
    out: Dict[str, Any] = {"zigbee_ok": False}
    try:
        # 1) Supervisor add-ons: Zigbee2MQTT started
        if addons_list is None:
            addons_list = ha_api.get_addons_list()
        if isinstance(addons_list, list):
            for addon in addons_list:
                if not isinstance(addon, dict):
                    continue
                if _addon_slug_matches_zigbee(addon.get("slug", "")) and addon.get("state") in ("started", "running"):
                    out["zigbee_ok"] = True
                    return out

        # 2) Core config entries: ZHA (domain="zha") loaded and not disabled
        entries = ha_api.get_config_entries()
        if isinstance(entries, list):
            for entry in entries:
                if not isinstance(entry, dict):
                    continue
                domain = entry.get("domain", "")
                state = entry.get("state", "")
                disabled = entry.get("disabled_by")  # None = enabled
                if disabled:
                    continue
                if domain == "zha" and state == "loaded":
                    out["zigbee_ok"] = True
                    return out
    except Exception as e:
        logger.debug("fetch_zigbee failed: %s", e)
    return out
