"""
Status handler for ESP32: fetches system status and sends STATUS with optional KV.
KV are optional: when available we send core/sup/net/lan/wifi/ext; otherwise heartbeat only.
"""

import logging
import threading
import time
from typing import Optional, Callable, FrozenSet
from communication.esp32_comm import ESP32Comm, KV
from api.ha_api import HomeAssistantAPI
from handlers.status_fetchers import get_all_status

logger = logging.getLogger(__name__)

# (ESP32 key, status dict key). Order preserved for consistent messages.
STATUS_KEY_MAP = (
    ("core", "core_ok"),
    ("sup", "supervisor_ok"),
    ("net", "network_ok"),
    ("lan", "lan_ok"),
    ("wifi", "wifi_ok"),
    ("ext", "exposed_ok"),
    ("zigbee", "zigbee_ok"),
    ("thread", "thread_ok"),
    ("matter", "matter_ok"),
)


class StatusHandler:
    """Handler for system status updates to ESP32 (STATUS message with optional KV)."""

    def __init__(
        self,
        esp32_comm: Optional[ESP32Comm] = None,
        ha_api: Optional[HomeAssistantAPI] = None,
        update_interval: float = 30.0,
        exposed_addon_slugs: Optional[FrozenSet[str]] = None,
    ) -> None:
        """
        Initialize status handler.

        Args:
            esp32_comm: ESP32 communication handler.
            ha_api: Home Assistant API client.
            update_interval: Interval in seconds between status sends (default 30).
            exposed_addon_slugs: Slugs for "exposed" add-ons; None = use default.
        """
        self.esp32_comm = esp32_comm
        self.ha_api = ha_api
        self.update_interval = update_interval
        self.exposed_addon_slugs = exposed_addon_slugs
        self.last_update = 0.0
        self.timer: Optional[threading.Timer] = None
        self._is_connected_callback: Optional[Callable[[], bool]] = None

    def set_connection_check(self, callback: Callable[[], bool]) -> None:
        """Set callback to check if ESP32 is connected."""
        self._is_connected_callback = callback

    def force_update(self) -> bool:
        """Force immediate status send (e.g. at connection). Returns True if sent."""
        return self._send_status()

    def start_periodic_updates(self) -> None:
        """Start periodic status updates via threading.Timer."""
        logger.info("Starting periodic status updates (interval=%.1fs)", self.update_interval)
        self._schedule_next()

    def stop_periodic_updates(self) -> None:
        """Stop periodic status updates."""
        if self.timer:
            logger.info("Stopping periodic status updates")
            self.timer.cancel()
            self.timer = None

    def _schedule_next(self) -> None:
        """Schedule next status send."""
        if self.timer:
            self.timer.cancel()
        self.timer = threading.Timer(self.update_interval, self._periodic_update)
        self.timer.start()
        logger.debug("Status update scheduled in %.1f seconds", self.update_interval)

    def _periodic_update(self) -> None:
        """Timer callback: send status if connected, then reschedule."""
        if self._is_connected_callback and not self._is_connected_callback():
            logger.debug("Status periodic update skipped: ESP32 not connected")
            return
        self._send_status()
        self._schedule_next()

    def _send_status(self) -> bool:
        """
        Fetch status from fetchers, build optional KV, send STATUS to ESP32.
        KV are optional: if fetch succeeds we send key=value; otherwise heartbeat only (no KV).
        """
        if not self.esp32_comm:
            logger.debug("StatusHandler._send_status() skipped: esp32_comm is None")
            return False
        kv_list: list = []
        try:
            if self.ha_api:
                status = get_all_status(self.ha_api, self.exposed_addon_slugs)
                for esp32_key, status_key in STATUS_KEY_MAP:
                    if status_key in status:
                        kv_list.append(KV(esp32_key, "1" if status[status_key] else "0"))
            if kv_list:
                msg_id = self.esp32_comm.send_command("STATUS", kv_list)
                payload = " ".join(f"{k.key}={k.val}" for k in kv_list)
                logger.info("STATUS sent (msg_id=%d): %s", msg_id, payload)
            else:
                msg_id = self.esp32_comm.send_command("STATUS")
                logger.info("STATUS sent (msg_id=%d): heartbeat (no KV)", msg_id)
            self.last_update = time.time()
            return msg_id > 0
        except Exception as e:
            logger.warning("Status fetch/send failed, sending heartbeat only: %s", e)
            try:
                msg_id = self.esp32_comm.send_command("STATUS")
                return msg_id > 0
            except Exception as e2:
                logger.error("Failed to send STATUS: %s", e2, exc_info=True)
                return False
