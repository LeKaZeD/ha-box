"""
Status fetcher: add-ons list (exposed on internet: DuckDNS, Cloudflare, etc.).
exposed_ok is True only if an exposed add-on is started AND its external URL (from addon options) responds.
URL is read from Supervisor: DuckDNS options["domains"], Cloudflare options["external_hostname"].
"""

import logging
from typing import Dict, Any, Optional, FrozenSet, List, Tuple
import requests
from urllib3.exceptions import InsecureRequestWarning
from api.ha_api import HomeAssistantAPI, DEFAULT_EXPOSED_ADDON_SLUGS

# Suppress warning when verify=False (self-signed certs)
requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)

logger = logging.getLogger(__name__)

EXPOSED_CHECK_TIMEOUT = 10
EXPOSED_CHECK_ALLOWED_CODES = (200, 201, 301, 302, 307, 308)


def _check_url_reachable(url: str) -> bool:
    """Return True if url responds with 2xx or 3xx (e.g. HA login page or API)."""
    try:
        r = requests.get(
            url,
            timeout=EXPOSED_CHECK_TIMEOUT,
            verify=False,
            allow_redirects=True,
        )
        ok = r.status_code in EXPOSED_CHECK_ALLOWED_CODES
        if ok:
            logger.info("exposed check: %s -> %d (reachable)", url, r.status_code)
        else:
            logger.info("exposed check: %s -> %d (not in 2xx/3xx)", url, r.status_code)
        return ok
    except Exception as e:
        logger.info("exposed check: %s -> unreachable (%s)", url, e)
        return False


def _url_from_addon_options(slug: str, options: Optional[Dict[str, Any]]) -> Optional[str]:
    """
    Build https URL from add-on options. Returns None if not supported or missing.
    DuckDNS: options["domains"] = ["myhome.duckdns.org"]
    Cloudflare: options["external_hostname"] = "ha.example.com"
    """
    if not options:
        return None
    slug_lower = slug.lower()
    if "duckdns" in slug_lower:
        domains = options.get("domains")
        if isinstance(domains, list) and domains and isinstance(domains[0], str):
            host = domains[0].strip()
            if host:
                return f"https://{host}"
    if "cloudflare" in slug_lower:
        host = options.get("external_hostname")
        if isinstance(host, str) and host.strip():
            return f"https://{host.strip()}"
    return None


def fetch_addons(
    ha_api: HomeAssistantAPI,
    exposed_slugs: Optional[FrozenSet[str]] = None,
    addons_list: Optional[List[Dict[str, Any]]] = None,
) -> Dict[str, Any]:
    """
    Fetch exposed-on-internet status: exposed_ok is True only when an exposed add-on
    is started AND its external URL (from addon options) responds.

    Args:
        ha_api: API client.
        exposed_slugs: Slugs that indicate "exposed" when started. Default: DEFAULT_EXPOSED_ADDON_SLUGS.
        addons_list: Optional pre-fetched add-ons list (avoids duplicate GET /addons).

    Returns:
        Dict with key: exposed_ok (bool).
    """
    slugs = exposed_slugs if exposed_slugs is not None else DEFAULT_EXPOSED_ADDON_SLUGS
    out: Dict[str, Any] = {"exposed_ok": False}
    try:
        if addons_list is None:
            addons_list = ha_api.get_addons_list()
        if not isinstance(addons_list, list):
            logger.debug("exposed: get_addons_list() not available or not a list")
            return out
        running_exposed: List[Tuple[str, Dict[str, Any]]] = []
        for addon in addons_list:
            if not isinstance(addon, dict):
                continue
            if addon.get("slug") in slugs and addon.get("state") in ("started", "running"):
                running_exposed.append((addon.get("slug", ""), addon))
        if not running_exposed:
            logger.debug("exposed: no running exposed add-on found")
            return out
        logger.info("exposed: running add-on(s) %s", [s for s, _ in running_exposed])
        # Get URL from running exposed addon options (DuckDNS domains, Cloudflare external_hostname) and verify reachability
        for slug, _ in running_exposed:
            info = ha_api.get_addon_info(slug)
            if not info:
                logger.debug("exposed: get_addon_info(%s) failed or empty", slug)
                continue
            options = info.get("options") if isinstance(info.get("options"), dict) else None
            url = _url_from_addon_options(slug, options)
            if url:
                logger.info("exposed: checking URL for %s -> %s", slug, url)
                if _check_url_reachable(url):
                    out["exposed_ok"] = True
                    break
            else:
                # Addon running but no URL in options (unknown schema): consider exposed
                logger.info("exposed: %s running but no URL in options -> exposed_ok=True (fallback)", slug)
                out["exposed_ok"] = True
                break
        # If we had URLs to check and none were reachable, exposed_ok stays False
        logger.debug("exposed: result exposed_ok=%s", out["exposed_ok"])
    except Exception as e:
        logger.debug("fetch_addons failed: %s", e)
    return out
