# Home Assistant App: HA Box

_Physical interface for Home Assistant with E-Paper display, sensors and controllers._

![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

## About

HA Box is a Home Assistant OS App that allows creating a physical interface for your Home Assistant installation via:

- **3.7" E-Paper Display** (GDEY037T03-FT21) with integrated front-light
- **Touch Interface** (FT6336U integrated)
- **NFC Reader** (PN7161, on ESP32)
- **Environmental Sensor** (BME280 - temperature, humidity, pressure)
- **LED Strip** (WS2812B - optional)
- **PWM Fan** (optional)

## Installation

1. Add this repository to your Home Assistant Apps
2. Install the "HA Box" App
3. Configure options according to your hardware
4. Start the App

## Configuration

See the complete documentation in the main repository for detailed configuration.

### Hardware Prerequisites

- Raspberry Pi 4 or 5
- GDEY037T03-FT21 E-Paper Display
- BME280 Sensor
- PN7161 NFC Module (optional, on ESP32)
- WS2812B LED Strip (optional)
- 5V PWM Fan (optional)

### Raspberry Pi Configuration

Enable I2C and SPI in `/mnt/boot/config.txt`:

```ini
dtparam=i2c_arm=on
dtparam=i2c1=on
dtparam=spi=on
```

## Support

For issues, questions or contributions, see the main project repository.

## License

Apache License 2.0
