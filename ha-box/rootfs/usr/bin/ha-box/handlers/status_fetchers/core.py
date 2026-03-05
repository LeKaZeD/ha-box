"""
Status fetcher: HA Core availability.
"""

import logging
from typing import Dict, Any
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


def fetch_core(ha_api: HomeAssistantAPI) -> Dict[str, Any]:
    """
    Fetch Core status (Home Assistant API reachable).

    Returns:
        Dict with key: core_ok (bool).
    """
    out: Dict[str, Any] = {"core_ok": False}
    try:
        out["core_ok"] = ha_api._check_core_available()
    except Exception as e:
        logger.debug("fetch_core failed: %s", e)
    return out
