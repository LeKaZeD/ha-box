"""
Day/Night mode handler for ESP32.

Synchronises day/night mode between Home Assistant and the ESP32 display.

Logic:
- On connection: reads sun.sun and sends the current mode to ESP32.
- Periodic: polls sun.sun every update_interval seconds and switches
  automatically at sunrise/sunset.
- Manual override: when the user touches the day/night switch on the ESP32,
  the mode is pinned until the next real sun event (sunrise or sunset).
  The override is tracked in memory only; an addon restart resets it.
- On every mode change: fires the HA event ha_box_mode_changed with
  {mode: "day" | "night"} and updates binary_sensor.ha_box_day_mode.
"""

import logging
import threading
from typing import Optional, Callable

from communication.esp32_comm import ESP32Comm, KV
from api.ha_api import HomeAssistantAPI

logger = logging.getLogger(__name__)

MODE_DAY   = 0
MODE_NIGHT = 1

_SUN_ENTITY = "sun.sun"
_SUN_ABOVE  = "above_horizon"
_EVENT_TYPE = "ha_box_mode_changed"


class DayNightHandler:
    """Manages day/night mode sync between HA (sun.sun) and ESP32."""

    def __init__(
        self,
        esp32_comm: Optional[ESP32Comm] = None,
        ha_api: Optional[HomeAssistantAPI] = None,
        update_interval: float = 60.0,
    ) -> None:
        self.esp32_comm = esp32_comm
        self.ha_api = ha_api
        self.update_interval = update_interval

        self._current_mode: Optional[int] = None   # last mode sent to ESP32
        self._last_sun_mode: Optional[int] = None  # last mode read from sun.sun
        self._manual_override: Optional[int] = None  # set when user overrides from ESP32

        self._timer: Optional[threading.Timer] = None
        self._is_connected_callback: Optional[Callable[[], bool]] = None

    def set_connection_check(self, callback: Callable[[], bool]) -> None:
        self._is_connected_callback = callback

    def force_update(self) -> bool:
        """Send current mode immediately (called on connection)."""
        return self._sync()

    def start_periodic_updates(self) -> None:
        logger.info("Starting periodic day/night updates (interval=%.0fs)", self.update_interval)
        self._schedule_next()

    def stop_periodic_updates(self) -> None:
        if self._timer:
            self._timer.cancel()
            self._timer = None

    # ------------------------------------------------------------------
    # Called by MessageHandler when ESP32 sends DAYMODE (user touched widget)
    # ------------------------------------------------------------------
    def on_esp32_daymode(self, mode: int) -> None:
        """Handle DAYMODE message from ESP32 (user touched the day/night switch)."""
        mode = max(0, min(1, mode))
        label = "day" if mode == MODE_DAY else "night"
        logger.info("ESP32 day/night switch: mode=%d (%s) — manual override active until next sun event", mode, label)
        self._manual_override = mode
        self._current_mode = mode
        self._notify_ha(mode)

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------
    def _schedule_next(self) -> None:
        if self._timer:
            self._timer.cancel()
        self._timer = threading.Timer(self.update_interval, self._periodic_update)
        self._timer.daemon = True
        self._timer.start()

    def _periodic_update(self) -> None:
        if self._is_connected_callback and not self._is_connected_callback():
            logger.debug("Day/night periodic update skipped: ESP32 not connected")
            self._schedule_next()
            return
        self._sync()
        self._schedule_next()

    def _sync(self) -> bool:
        """Determine current mode and send to ESP32 if changed."""
        if not self.esp32_comm or not self.ha_api:
            return False

        mode = self._determine_mode()
        if mode is None:
            return False

        if mode == self._current_mode:
            logger.debug("Day/night mode unchanged (%s), skipping send", "day" if mode == MODE_DAY else "night")
            return True

        return self._send_mode(mode)

    def _determine_mode(self) -> Optional[int]:
        """
        Returns the target mode (MODE_DAY or MODE_NIGHT).

        Reads sun.sun for the authoritative mode. If a real sun event occurred
        (sun mode changed since last poll), clears any manual override.
        Otherwise, honours the manual override if set.
        """
        if self.ha_api is None:
            logger.warning("Could not determine day/night mode (no HA API)")
            return None
        sun = self.ha_api.get_state(_SUN_ENTITY)
        if sun is None:
            logger.warning("Could not determine day/night mode (sun.sun unavailable)")
            return None

        sun_mode = MODE_DAY if sun.get("state") == _SUN_ABOVE else MODE_NIGHT

        # Detect real sun transition → clear manual override
        if self._last_sun_mode is not None and sun_mode != self._last_sun_mode:
            if self._manual_override is not None:
                logger.info(
                    "Sun event detected (%s → %s): clearing manual override",
                    "day" if self._last_sun_mode == MODE_DAY else "night",
                    "day" if sun_mode == MODE_DAY else "night",
                )
            self._manual_override = None

        self._last_sun_mode = sun_mode

        if self._manual_override is not None:
            logger.debug("Manual override active: mode=%d", self._manual_override)
            return self._manual_override

        return sun_mode

    def _send_mode(self, mode: int) -> bool:
        """Send DAYMODE to ESP32 and notify HA."""
        if self.esp32_comm is None:
            logger.warning("Cannot send DAYMODE: no ESP32 comm")
            return False
        try:
            msg_id = self.esp32_comm.send_command("DAYMODE", [KV("mode", str(mode))])
            label = "day" if mode == MODE_DAY else "night"
            logger.info("DAYMODE sent to ESP32: mode=%d (%s)", mode, label)
            self._current_mode = mode
            self._notify_ha(mode)
            return msg_id > 0
        except Exception as e:
            logger.error("Failed to send DAYMODE: %s", e, exc_info=True)
            return False

    def _notify_ha(self, mode: int) -> None:
        """Update binary_sensor and fire event in HA."""
        if not self.ha_api:
            return
        label = "day" if mode == MODE_DAY else "night"
        self.ha_api.update_day_mode_sensor(mode)
        self.ha_api.fire_event(_EVENT_TYPE, {"mode": label})
