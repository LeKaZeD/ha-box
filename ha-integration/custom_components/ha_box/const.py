"""Constants for HA Box integration."""

DOMAIN = "ha_box"

# Events fired by the HA Box addon
EVENT_SENSORS     = "ha_box_sensors"       # {tC, hum, pPa}
EVENT_CASE_TEMP   = "ha_box_case_temp"     # {tC}
EVENT_MODE        = "ha_box_mode_changed"  # {mode: "day"|"night"}

# Device info
DEVICE_NAME         = "HA Box"
DEVICE_MANUFACTURER = "HA Box"
DEVICE_MODEL        = "ESP32 Smart Hub"
DEVICE_IDENTIFIER   = "ha_box_main"
