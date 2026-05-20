"""Sensor platform for HA Box integration."""

from __future__ import annotations

import logging
from typing import Any, Callable

from homeassistant.components.sensor import (
    SensorDeviceClass,
    SensorEntity,
    SensorStateClass,
)
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import PERCENTAGE, UnitOfPressure, UnitOfTemperature
from homeassistant.core import Event, HomeAssistant, callback
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import (
    DEVICE_IDENTIFIER,
    DEVICE_MANUFACTURER,
    DEVICE_MODEL,
    DEVICE_NAME,
    DOMAIN,
    EVENT_CASE_TEMP,
    EVENT_MODE,
    EVENT_SENSORS,
)

_LOGGER = logging.getLogger(__name__)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    entities = [
        HABoxSensor(
            unique_id="ha_box_temperature",
            name="Temperature",
            device_class=SensorDeviceClass.TEMPERATURE,
            unit=UnitOfTemperature.CELSIUS,
            event_type=EVENT_SENSORS,
            event_key="tC",
        ),
        HABoxSensor(
            unique_id="ha_box_humidity",
            name="Humidity",
            device_class=SensorDeviceClass.HUMIDITY,
            unit=PERCENTAGE,
            event_type=EVENT_SENSORS,
            event_key="hum",
        ),
        HABoxSensor(
            unique_id="ha_box_pressure",
            name="Pressure",
            device_class=SensorDeviceClass.ATMOSPHERIC_PRESSURE,
            unit=UnitOfPressure.HPA,
            event_type=EVENT_SENSORS,
            event_key="pPa",
            value_fn=lambda v: round(float(v) / 100.0, 2),
        ),
        HABoxSensor(
            unique_id="ha_box_case_temperature",
            name="Case Temperature",
            device_class=SensorDeviceClass.TEMPERATURE,
            unit=UnitOfTemperature.CELSIUS,
            event_type=EVENT_CASE_TEMP,
            event_key="tC",
        ),
    ]
    async_add_entities(entities + [HABoxDayModeSensor()])


class HABoxSensor(SensorEntity):
    """A sensor entity updated by HA Box addon events."""

    _attr_has_entity_name = True
    _attr_state_class = SensorStateClass.MEASUREMENT
    _attr_should_poll = False

    def __init__(
        self,
        unique_id: str,
        name: str,
        device_class: SensorDeviceClass,
        unit: str,
        event_type: str,
        event_key: str,
        value_fn: Callable[[Any], float | int] | None = None,
    ) -> None:
        self._attr_unique_id = unique_id
        self._attr_name = name
        self._attr_device_class = device_class
        self._attr_native_unit_of_measurement = unit
        self._event_type = event_type
        self._event_key = event_key
        self._value_fn = value_fn or (lambda v: round(float(v), 2))
        self._attr_native_value = None

    @property
    def device_info(self) -> DeviceInfo:
        return DeviceInfo(
            identifiers={(DOMAIN, DEVICE_IDENTIFIER)},
            name=DEVICE_NAME,
            manufacturer=DEVICE_MANUFACTURER,
            model=DEVICE_MODEL,
        )

    async def async_added_to_hass(self) -> None:
        @callback
        def handle_event(event: Event) -> None:
            val = event.data.get(self._event_key)
            if val is not None:
                try:
                    self._attr_native_value = self._value_fn(val)
                    self.async_write_ha_state()
                except (ValueError, TypeError) as e:
                    _LOGGER.warning("Failed to parse value for %s: %r — %s", self._event_key, val, e)

        self.async_on_remove(
            self.hass.bus.async_listen(self._event_type, handle_event)
        )


class HABoxDayModeSensor(SensorEntity):
    """Sensor showing current day/night mode as text."""

    _attr_has_entity_name = True
    _attr_name = "Mode"
    _attr_unique_id = "ha_box_day_mode"
    _attr_should_poll = False
    _attr_device_class = SensorDeviceClass.ENUM
    _attr_options = ["Day", "Night"]
    _attr_native_value = None

    @property
    def device_info(self) -> DeviceInfo:
        return DeviceInfo(
            identifiers={(DOMAIN, DEVICE_IDENTIFIER)},
            name=DEVICE_NAME,
            manufacturer=DEVICE_MANUFACTURER,
            model=DEVICE_MODEL,
        )

    async def async_added_to_hass(self) -> None:
        @callback
        def handle_event(event: Event) -> None:
            mode = event.data.get("mode")
            if mode == "day":
                self._attr_native_value = "Day"
            elif mode == "night":
                self._attr_native_value = "Night"
            else:
                return
            self.async_write_ha_state()

        self.async_on_remove(
            self.hass.bus.async_listen(EVENT_MODE, handle_event)
        )
