"""
Status fetcher: Matter (Matter integration in Core or Matter add-on).
Detects add-ons whose slug contains "matter", or Core entities with domain "matter".
"""

import logging
from typing import Dict, Any, Optional, List
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)

MATTER_ENTITY_PREFIX = "matter."


def _addon_slug_matches_matter(slug: str) -> bool:
    """True if this add-on slug indicates Matter."""
    if not slug:
        return False
    return "matter" in slug.lower()


def fetch_matter(ha_api: HomeAssistantAPI, addons_list: Optional[List[Dict[str, Any]]] = None) -> Dict[str, Any]:
    """
    Detect Matter: add-on running or Matter integration loaded in Core.

    Args:
        ha_api: API client.
        addons_list: Optional pre-fetched add-ons list (avoids duplicate GET /addons).

    Returns:
        Dict with key: matter_ok (bool).
    """
    out: Dict[str, Any] = {"matter_ok": False}
    try:
        # 1) Add-ons: any Matter add-on started
        if addons_list is None:
            addons_list = ha_api.get_addons_list()
        if isinstance(addons_list, list):
            for addon in addons_list:
                if not isinstance(addon, dict):
                    continue
                slug = addon.get("slug", "")
                state = addon.get("state", "")
                if _addon_slug_matches_matter(slug) and state in ("started", "running"):
                    out["matter_ok"] = True
                    return out
        # 2) Core: Matter integration entities
        states = ha_api.get_all_states()
        if isinstance(states, list):
            for item in states:
                eid = item.get("entity_id") if isinstance(item, dict) else None
                if isinstance(eid, str) and eid.startswith(MATTER_ENTITY_PREFIX):
                    out["matter_ok"] = True
                    return out
    except Exception as e:
        logger.debug("fetch_matter failed: %s", e)
    return out
