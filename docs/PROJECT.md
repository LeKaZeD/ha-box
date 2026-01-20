# HA Box - Home Assistant Add-on for Raspberry Pi

## Project Vision

**HA Box** is a Home Assistant OS add-on for Raspberry Pi that allows controlling a set of hardware devices:

- **3.7" E-Paper Display** (GDEY037T03-FT21) - Display Home Assistant information with integrated front-light
- **I2C Touch** (FT6336U integrated) - Navigation interface
- **I2C NFC Sensor** - NFC tag reading
- **I2C BME280 Sensor** - Temperature, humidity and atmospheric pressure
- **LED Strip** - Visual effects and notifications
- **PWM Fan** - Thermal regulation

## Main Objective

Provide a simple and elegant physical interface to display and interact with certain Home Assistant information, directly on a screen connected to the Raspberry Pi.

## Identified Technical Constraints

### Startup Order

The add-on can use the `startup` parameter in `config.yaml`:
- `initialize`: Starts very early, before other services
- `system`: Starts with system services
- `services`: Starts after system services
- `application`: Starts after Home Assistant (default)

**Important**: Add-ons are managed by the Supervisor, which starts after OS boot. An add-on cannot start before the system itself.

### Hardware Access

To access SPI/I2C/GPIO buses, you need to:
1. Enable interfaces in Raspberry Pi `config.txt`
2. Declare devices in add-on `config.yaml`
3. Potentially disable "Protection Mode"

### Reference Existing Add-ons

- [ha-rpi_gpio](https://github.com/thecode/ha-rpi_gpio) - GPIO access
- [Pironman](https://github.com/sunfounder/home-assistant-addon) - Display/LED/fan management
- [HassOS I2C Configurator](https://community.home-assistant.io/t/add-on-hassos-i2c-configurator/264167) - I2C configuration

## Project Status

**In definition** - Design and documentation phase

---

*Last updated: 2026-01-17*
