"""
Status fetcher: network interfaces (LAN / WiFi).
"""

import logging
from typing import Dict, Any
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)


def fetch_network(ha_api: HomeAssistantAPI) -> Dict[str, Any]:
    """
    Fetch network status from Supervisor (interfaces: ethernet, wireless).

    Returns:
        Dict with keys: network_ok, lan_ok, wifi_ok (bool).
    """
    out: Dict[str, Any] = {
        "network_ok": False,
        "lan_ok": False,
        "wifi_ok": False,
    }
    try:
        net = ha_api.get_network_info()
        if net is None or not isinstance(net, dict):
            return out
        interfaces = net.get("interfaces")
        if interfaces is None and isinstance(net.get("data"), dict):
            interfaces = net["data"].get("interfaces")
        if interfaces is None and isinstance(net.get("data"), list):
            interfaces = net["data"]
        if not isinstance(interfaces, list):
            return out
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
    except Exception as e:
        logger.debug("fetch_network failed: %s", e)
    return out
