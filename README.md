# HA Box - Home Assistant Add-on for Raspberry Pi

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

**HA Box** is a Home Assistant OS add-on for Raspberry Pi that allows controlling a set of hardware devices to create a physical interface with Home Assistant.

## Objective

Display and interact with Home Assistant information via a touchscreen connected to the Raspberry Pi, while managing local sensors and actuators.

## Features

| Feature | Interface | Hardware | Status |
|---------|-----------|----------|--------|
| E-Paper Display | SPI | GDEY037T03-FT21 | Planned |
| Touch Interface | I2C | FT6336U (integrated) | Planned |
| NFC Reader | I2C | PN532 | Planned |
| BME280 Sensor | I2C | Temperature/Humidity/Pressure | Planned |
| LED Strip | GPIO | WS2812B | To be defined |
| Fan | PWM | 5V PWM | To be defined |

## Prerequisites

### Hardware

- Raspberry Pi 4 or 5
- Home Assistant OS installed
- **E-Paper Display**: GDEY037T03-FT21 (3.7", 240×416, integrated touch)
- **Environmental Sensor**: BME280 (temperature, humidity, pressure)
- **NFC Module**: PN532 (I2C)
- WS2812B LED strip (optional)
- 5V PWM fan (optional)

**See [docs/HARDWARE.md](docs/HARDWARE.md) for detailed specifications**

### Raspberry Pi Configuration

Before installing the add-on, enable I2C and SPI in `/mnt/boot/config.txt`:

```ini
dtparam=i2c_arm=on
dtparam=i2c1=on
dtparam=spi=on
```

## Installation

1. Add this repository to your Home Assistant add-ons:

   [![Add repository](https://my.home-assistant.io/badges/supervisor_add_addon_repository.svg)](https://my.home-assistant.io/redirect/supervisor_add_addon_repository/?repository_url=https%3A%2F%2Fgithub.com%2FVOTRE_USERNAME%2Fha-box)

   Or manually: **Settings** → **Add-ons** → **Add-on Store** → **⋮** → **Repositories** → Add the repository URL

2. Install the "HA Box" add-on
3. Configure options according to your hardware
4. Start the add-on

## Configuration

```yaml
# Example configuration (coming soon)
display:
  type: "ili9341"
  rotation: 0
  
sensors:
  temperature:
    enabled: true
    address: 0x76
  nfc:
    enabled: true
    address: 0x24

entities:
  - sensor.temperature_salon
  - sensor.humidity_salon
  - switch.lumiere_salon
```

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

**Infrastructure complete** - Base infrastructure is implemented and tested. Hardware drivers development (Phase 3) is in progress.

### Roadmap

- [x] Initial documentation
- [x] Technical architecture
- [x] Features specification
- [x] Base infrastructure (HAL, managers, API client)
- [x] Security documentation and AppArmor profile
- [x] Presentation assets (icon, logo)
- [ ] SPI display prototype (Phase 3)
- [ ] I2C sensor integration (Phase 3)
- [ ] Touch interface (Phase 3)
- [ ] First alpha release

## Useful Resources

- [Home Assistant Add-ons Documentation](https://developers.home-assistant.io/docs/add-ons)
- [ha-rpi_gpio](https://github.com/thecode/ha-rpi_gpio) - Reference GPIO add-on
- [Pironman](https://github.com/sunfounder/home-assistant-addon) - Similar add-on by SunFounder

## License

This project is licensed under [Apache 2.0](LICENSE).

---

*Project started on January 17, 2026*
