# HA Box – Home Assistant Add-on

_UART bridge between the haboxesp ESP32 board and Home Assistant._

![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

## About

The HA Box add-on is one component of the [HA Box project](../README.md). It runs as a Home Assistant OS Supervisor App on the Raspberry Pi and acts as a bridge between the ESP32 (haboxesp board, connected over UART) and Home Assistant (Supervisor API + Core API).

The add-on does not access any hardware directly. All sensors, display, touch, NFC, fan, and button logic run on the ESP32. The add-on sends status updates to the ESP32 and handles requests coming from it (e.g. shutdown).

## Prerequisites

- Raspberry Pi 4 or 5 running Home Assistant OS
- haboxesp board connected via UART (`/dev/serial0`)
- UART enabled in `/mnt/boot/config.txt`:

```ini
enable_uart=1
dtoverlay=minimal-bt #on Raspberry PI4 only
```

Reboot after editing. See [docs/ESP32_RASP_COM.md](../docs/ESP32_RASP_COM.md) for details.

## Installation

1. Add the HA Box repository to Home Assistant Apps
2. Install **HA Box**
3. Configure options (see below)
4. Start the add-on

## Configuration

```yaml
home_assistant:
  weather_entity: "weather.forecast_home"
  update_interval: 300

display:
  enabled: true
  rotation: 0
  refresh_mode: "full"
  front_light:
    enabled: true
    brightness: 50

sensors:
  bme280:
    enabled: true
    address: "auto"
    update_interval: 30
  nfc:
    enabled: true
    address: "0x24"
    polling_interval: 1
  touch:
    enabled: true

control:
  fan:
    enabled: false
    pin: 18
    auto_control: true
    min_temp: 40
    max_temp: 60
  led:
    enabled: false
    pin: 21
    count: 10
```

Language is auto-detected from the Home Assistant Core configuration (`/api/config` → `language`), with fallback to the container environment (`LANG`) and then English.

## Support

For issues, questions, or contributions, see the [main repository](../README.md) and [CONTRIBUTING.md](../CONTRIBUTING.md).

## License

Apache License 2.0
