"""
Status fetcher: Supervisor info.
"""

import logging
from typing import Dict, Any
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


def fetch_supervisor(ha_api: HomeAssistantAPI) -> Dict[str, Any]:
    """
    Fetch Supervisor status.

    Returns:
        Dict with key: supervisor_ok (bool).
    """
    out: Dict[str, Any] = {"supervisor_ok": False}
    try:
        result = ha_api.get_supervisor_info()
        if result is not None and isinstance(result, dict):
            out["supervisor_ok"] = result.get("result") == "ok"
    except Exception as e:
        logger.debug("fetch_supervisor failed: %s", e)
    return out
