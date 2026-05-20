"""
Status fetcher: HA warnings and critical errors.

- errors_count  : Supervisor "unhealthy" states (system truly broken)
- warnings_count: active persistent notifications (persistent_notification.* with state=notifying)
"""

import logging
from typing import Dict, Any
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


def fetch_alerts(ha_api: HomeAssistantAPI) -> Dict[str, Any]:
    """
    Count warnings and errors from Supervisor unhealthy states + Persistent Notifications.

    - errors_count  : Supervisor "unhealthy" entries (system truly broken)
    - warnings_count: active persistent notifications (state=notifying)

    Returns:
        Dict with keys: warnings_count (int), errors_count (int).
    """
    out: Dict[str, Any] = {"warnings_count": 0, "errors_count": 0}
    try:
        issues = ha_api.get_repair_issues() or []
        for issue in issues:
            severity = issue.get("severity", "")
            if severity in ("critical", "error"):
                out["errors_count"] += 1
                logger.debug("fetch_alerts: critical issue type=%s", issue.get("type", "?"))
            elif severity == "warning":
                out["warnings_count"] += 1
                logger.debug("fetch_alerts: warning issue type=%s", issue.get("type", "?"))
            # unknown severity: ignore (future-proofing)
    except Exception as e:
        logger.debug("fetch_alerts (repair issues) failed: %s", e)
    try:
        notifs = ha_api.get_persistent_notifications() or []
        if notifs:
            for n in notifs:
                logger.debug("fetch_alerts: notif entity=%s", n.get("entity_id", "?"))
        out["warnings_count"] += len(notifs)
    except Exception as e:
        logger.debug("fetch_alerts (persistent notifications) failed: %s", e)
    logger.debug(
        "fetch_alerts: warnings=%d errors=%d",
        out["warnings_count"],
        out["errors_count"],
    )
    return out
