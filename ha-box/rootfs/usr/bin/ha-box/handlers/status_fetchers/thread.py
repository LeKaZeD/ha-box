"""
Status fetcher: Thread (OpenThread Border Router, Silabs Multiprotocol, etc.).
- OpenThread Border Router (OTBR): add-on slug contains "openthread" or "otbr", or Core entities "otbr.*".
- Silabs Multiprotocol: add-on slug contains "silabs" (multipan Zigbee+Thread).
- Any other: slug contains "thread".
"""

import logging
from typing import Dict, Any, Optional, List
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


def _addon_slug_matches_thread(slug: str) -> bool:
    """True if this add-on slug indicates Thread (OTBR, Silabs multiprotocol, etc.)."""
    if not slug:
        return False
    s = slug.lower()
    return "openthread" in s or "otbr" in s or "silabs" in s or "thread" in s


def fetch_thread(ha_api: HomeAssistantAPI, addons_list: Optional[List[Dict[str, Any]]] = None) -> Dict[str, Any]:
    """
    Detect Thread: OpenThread Border Router or Silabs Multiprotocol add-on, or OTBR entities in Core.

    Args:
        ha_api: API client.
        addons_list: Optional pre-fetched add-ons list (avoids duplicate GET /addons).

    Returns:
        Dict with key: thread_ok (bool).
    """
    out: Dict[str, Any] = {"thread_ok": False}
    try:
        # 1) Add-ons: OTBR, Silabs Multiprotocol, or any thread-related add-on
        if addons_list is None:
            addons_list = ha_api.get_addons_list()
        if isinstance(addons_list, list):
            for addon in addons_list:
                if not isinstance(addon, dict):
                    continue
                slug = addon.get("slug", "")
                state = addon.get("state", "")
                if _addon_slug_matches_thread(slug) and state in ("started", "running"):
                    out["thread_ok"] = True
                    return out
        # No config entry fallback for Thread: HAOS always creates "thread" and "otbr" entries
        # automatically regardless of whether a border router is actually active.
        # Add-on detection above is the only reliable indicator.
    except Exception as e:
        logger.debug("fetch_thread failed: %s", e)
    return out
