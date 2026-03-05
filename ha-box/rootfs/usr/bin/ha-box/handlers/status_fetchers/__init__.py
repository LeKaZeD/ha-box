"""
Status fetchers: modular sources for system status (Core, Supervisor, network, addons, zigbee, thread, matter).
"""

import logging
from typing import Dict, Any, Optional, FrozenSet, List
from api.ha_api import HomeAssistantAPI

from handlers.status_fetchers.core import fetch_core
from handlers.status_fetchers.supervisor import fetch_supervisor
from handlers.status_fetchers.network import fetch_network
from handlers.status_fetchers.addons import fetch_addons
from handlers.status_fetchers.zigbee import fetch_zigbee
from handlers.status_fetchers.thread import fetch_thread
from handlers.status_fetchers.matter import fetch_matter

logger = logging.getLogger(__name__)


def get_all_status(
    ha_api: HomeAssistantAPI,
    exposed_addon_slugs: Optional[FrozenSet[str]] = None,
) -> Dict[str, bool]:
    """
    Run all status fetchers and merge into a single status dict.
    GET /addons is called once and the list is passed to fetchers that need it.

    Args:
        ha_api: API client.
        exposed_addon_slugs: Slugs for "exposed" add-ons (DuckDNS, Cloudflare, etc.).

    Returns:
        Dict with keys: core_ok, supervisor_ok, network_ok, lan_ok, wifi_ok, exposed_ok,
        zigbee_ok, thread_ok, matter_ok (bool).
    """
    out: Dict[str, bool] = {
        "core_ok": False,
        "supervisor_ok": False,
        "network_ok": False,
        "lan_ok": False,
        "wifi_ok": False,
        "exposed_ok": False,
        "zigbee_ok": False,
        "thread_ok": False,
        "matter_ok": False,
    }
    try:
        data = fetch_core(ha_api)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
        data = fetch_supervisor(ha_api)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
        data = fetch_network(ha_api)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
        # One GET /addons for all addon-based fetchers
        addons_list: Optional[List[Dict[str, Any]]] = ha_api.get_addons_list()
        if not isinstance(addons_list, list):
            addons_list = None
        data = fetch_addons(ha_api, exposed_addon_slugs, addons_list=addons_list)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
        data = fetch_zigbee(ha_api, addons_list=addons_list)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
        data = fetch_thread(ha_api, addons_list=addons_list)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
        data = fetch_matter(ha_api, addons_list=addons_list)
        out.update((k, v) for k, v in data.items() if isinstance(v, bool))
    except Exception as e:
        logger.warning("get_all_status failed: %s", e)
    logger.debug(
        "get_all_status: core=%s sup=%s net=%s lan=%s wifi=%s ext=%s zigbee=%s thread=%s matter=%s",
        out.get("core_ok"), out.get("supervisor_ok"), out.get("network_ok"),
        out.get("lan_ok"), out.get("wifi_ok"), out.get("exposed_ok"),
        out.get("zigbee_ok"), out.get("thread_ok"), out.get("matter_ok"),
    )
    return out
