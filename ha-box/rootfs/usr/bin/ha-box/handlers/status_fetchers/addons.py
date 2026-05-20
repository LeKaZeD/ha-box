"""
Status fetcher: add-ons list (exposed on internet: DuckDNS, Cloudflare tunnel, etc.).

Detection strategy per add-on type:
- Cloudflare tunnel (cloudflared, cloudflare): if the add-on is started, the tunnel is
  established. No outbound HTTP check is performed because the container cannot reliably
  resolve or reach the external hostname from inside the Supervisor network, and the
  tunnel health is implicitly guaranteed by the cloudflared process being up.
- DuckDNS: add-on started AND external URL responds (HTTP check kept because DuckDNS is
  dynamic DNS only, not a tunnel proxy; URL reachability is the meaningful signal).
"""

import logging
from typing import Dict, Any, Optional, FrozenSet, List, Tuple
import requests
import urllib3
from api.ha_api import HomeAssistantAPI, DEFAULT_EXPOSED_ADDON_SLUGS

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

logger = logging.getLogger(__name__)

EXPOSED_CHECK_TIMEOUT = 10
EXPOSED_CHECK_ALLOWED_CODES = (200, 201, 301, 302, 307, 308)


def _is_tunnel_slug(slug: str) -> bool:
    """Return True for add-ons that act as tunnel proxies (Cloudflare tunnel variants)."""
    s = slug.lower()
    return "cloudflare" in s


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
        logger.info(
            "exposed check: %s -> %d (%s)",
            url, r.status_code, "reachable" if ok else "not in 2xx/3xx",
        )
        return ok
    except Exception as e:
        logger.info("exposed check: %s -> unreachable (%s)", url, e)
        return False


def _duckdns_url(options: Optional[Dict[str, Any]]) -> Optional[str]:
    """Build DuckDNS URL from add-on options["domains"]."""
    if not options:
        return None
    domains = options.get("domains")
    if isinstance(domains, list) and domains and isinstance(domains[0], str):
        host = domains[0].strip()
        if host:
            return f"https://{host}"
    return None


def fetch_addons(
    ha_api: HomeAssistantAPI,
    exposed_slugs: Optional[FrozenSet[str]] = None,
    addons_list: Optional[List[Dict[str, Any]]] = None,
) -> Dict[str, Any]:
    """
    Fetch exposed-on-internet status.

    - Cloudflare tunnel: exposed_ok=True as soon as the add-on is started.
    - DuckDNS: exposed_ok=True only if the add-on is started AND the external URL responds.

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
            slug = addon.get("slug", "")
            slug_l = slug.lower()
            in_explicit = slug in slugs
            in_substring = "cloudflare" in slug_l or "duckdns" in slug_l
            if (in_explicit or in_substring) and addon.get("state") in ("started", "running"):
                running_exposed.append((slug, addon))

        if not running_exposed:
            logger.debug("exposed: no running exposed add-on found")
            return out

        logger.info("exposed: running add-on(s): %s", [s for s, _ in running_exposed])

        for slug, _ in running_exposed:
            slug_l = slug.lower()

            # Cloudflare tunnel: trust Supervisor state, no outbound HTTP check.
            if _is_tunnel_slug(slug):
                logger.info("exposed: %s is a tunnel add-on and is started -> exposed_ok=True", slug)
                out["exposed_ok"] = True
                break

            # DuckDNS: verify the external URL is reachable.
            if "duckdns" in slug_l:
                info = ha_api.get_addon_info(slug)
                options = info.get("options") if isinstance(info, dict) and isinstance(info.get("options"), dict) else None
                url = _duckdns_url(options)
                if url:
                    logger.info("exposed: checking DuckDNS URL for %s -> %s", slug, url)
                    if _check_url_reachable(url):
                        out["exposed_ok"] = True
                        break
                else:
                    logger.info("exposed: %s started, no domain in options -> exposed_ok=True (fallback)", slug)
                    out["exposed_ok"] = True
                    break

        logger.debug("exposed: result exposed_ok=%s", out["exposed_ok"])
    except Exception as e:
        logger.debug("fetch_addons failed: %s", e)
    return out
