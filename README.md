# HA Box - Home Assistant Add-on for Raspberry Pi

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

**HA Box** is a Home Assistant OS App for Raspberry Pi that provides a physical interface to Home Assistant using an ESP32, an E-Paper display and local sensors/actuators.

## Objective

Display and interact with Home Assistant information via a touchscreen connected to the Raspberry Pi, while managing local sensors and actuators.

## Features (current state)

| Feature | Interface | Hardware | Status |
|---------|-----------|----------|--------|
| E-Paper Display | SPI (on ESP32) | GDEY037T03-FT21 | Implemented (alpha) |
| Touch Interface | I2C (on ESP32) | FT6336U (integrated) | Implemented (basic UI) |
| Power Button + J2 | UART (ESP32 ↔ Pi) | Front power button + J2 header | Implemented (wake + shutdown) |
| BME280 Sensor | I2C (on ESP32) | Temperature/Humidity/Pressure | Implemented |
| NFC Reader | I2C (on ESP32) | PN7161 | Planned |
| Fan | PWM (on ESP32) | 5V PWM | Implemented (temperature-based) |
| Home Assistant status icons | UART (Pi ↔ ESP32) | — | Implemented (Core / Supervisor / Net / Zigbee / Thread / Matter) |

## Prerequisites

### Hardware

- Raspberry Pi 4 or 5
- Home Assistant OS installed
- **E-Paper Display**: GDEY037T03-FT21 (3.7", 240×416, integrated touch)
- **Environmental Sensor**: BME280 (temperature, humidity, pressure)
- **NFC Module**: PN7161 (I2C)
- 5V PWM fan (optional)

**See [docs/HARDWARE.md](docs/HARDWARE.md) for detailed specifications**

### Raspberry Pi Configuration (UART)

The Pi talks to the ESP32 over UART (`/dev/serial0`). On Home Assistant OS you must enable UART and (optionally) disable Bluetooth in `/mnt/boot/config.txt`:

```ini
enable_uart=1
dtoverlay=disable-bt
```

Then reboot the host. See `docs/ESP32_RASP_COM.md` for detailed steps and testing commands.

## Installation

1. Add this repository to your Home Assistant Apps:

   [![Add repository](https://my.home-assistant.io/badges/supervisor_add_addon_repository.svg)](https://my.home-assistant.io/redirect/supervisor_add_addon_repository/?repository_url=https%3A%2F%2Fgithub.com%2FLeKaZeD%2Fha-box)

   Or manually: **Settings** → **Add-ons** → **Add-on Store** → **⋮** → **Repositories** → Add the repository URL

2. Install the "HA Box" App
3. Configure options according to your hardware
4. Start the App

## Configuration (add-on options)

The add-on is configured via its options in Home Assistant. Current schema (see `ha-box/config.yaml`):

```yaml
home_assistant:
  weather_entity: "weather.forecast_home"
  update_interval: 300   # seconds

display:
  enabled: true
  rotation: 0            # 0–3
  refresh_mode: "full"   # or "partial"
  front_light:
    enabled: true
    brightness: 50       # 0–100

sensors:
  bme280:
    enabled: true
    address: "auto"      # "auto", "0x76" or "0x77"
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

Language is **auto-detected** from Home Assistant Core configuration (`/api/config` → `language`), with fallback to the container environment (`LANG` / `SUPERVISOR_LANGUAGE`) and then English.

## Documentation

| Document | Description |
|----------|-------------|
| [ROADMAP.md](ROADMAP.md) | **Roadmap** - Current status and next steps |
| [PROJECT.md](docs/PROJECT.md) | Project vision and objectives |
| [FEATURES.md](docs/FEATURES.md) | Features specification |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Technical architecture |
| [HARDWARE.md](docs/HARDWARE.md) | Detailed hardware specifications |
| [TECH_STACK.md](docs/TECH_STACK.md) | Detailed technical stack |
| [I18N.md](docs/I18N.md) | Multilingual support (i18n) |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution guide |
| [SECURITY.md](ha-box/SECURITY.md) | Security policy and rating |
| [TESTING.md](TESTING.md) | Testing and installation guide |

## Contributing

Contributions are welcome! See the [contribution guide](CONTRIBUTING.md) to get started.

### How to participate

1. Read the documentation in `docs/`
2. Report bugs via Issues
3. Propose features via Issues
4. Submit Pull Requests

## Project Status

**Core infrastructure and first hardware loop are implemented**: ESP32 ↔ Pi UART link, power button + shutdown flow, E-Paper UI, BME280 and fan control are working in an early alpha state.

### Roadmap

- [x] Initial documentation
- [x] Technical architecture
- [x] Features specification
- [x] Base infrastructure (HAL, managers, API client)
- [x] Security documentation and AppArmor profile
- [x] Presentation assets (icon, logo)
- [x] SPI E-Paper display integration (alpha)
- [x] I2C sensor integration (BME280)
- [x] Touch interface (basic navigation)
- [ ] NFC integration (PN7161)
- [ ] First public alpha release

## Useful Resources

- [Home Assistant Apps Documentation](https://developers.home-assistant.io/docs/apps/)
- [ha-rpi_gpio](https://github.com/thecode/ha-rpi_gpio) - Reference GPIO App
- [Pironman](https://github.com/sunfounder/home-assistant-addon) - Similar App by SunFounder

## License

This project is licensed under [Apache 2.0](LICENSE).

---

*Project started on January 17, 2026*
