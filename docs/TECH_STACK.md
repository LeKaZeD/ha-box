# Technical Stack - HA Box

This document details all technologies, languages and libraries used in the project.

## Languages

### Python 3.9+

**Main usage**: Business application, hardware drivers, communication with Home Assistant.

**Reasons for choice:**
- Excellent hardware library support (GPIO, I2C, SPI)
- Adafruit libraries available
- Easy to maintain and debug
- Native support in Home Assistant images

**Standards:**
- PEP 8 for style
- Type hints mandatory
- Google style docstrings
- Python 3.9+ minimum (Home Assistant compatibility)

### Bash 5.x

**Usage**: s6-overlay startup/shutdown scripts.

**Reasons for choice:**
- Standard for Linux system scripts
- Native integration with s6-overlay
- Access to bashio for Supervisor API

**Standards:**
- Shebang: `#!/usr/bin/with-contenv bashio`
- Indentation: 2 spaces
- Variables always quoted: `"${variable}"`

### YAML

**Usage**: Add-on configuration, build, translations.

**Files:**
- `config.yaml`: Add-on configuration
- `build.yaml`: Multi-arch build configuration
- `translations/*.yaml`: Translations

---

## Base Technologies

### Docker

**Usage**: Add-on containerization.

**Base image**: Official Home Assistant images
- `ghcr.io/home-assistant/{arch}-base:3.15`
- aarch64 support (Raspberry Pi)

**Reasons for choice:**
- Standard for Home Assistant Apps
- Isolation and security
- Reproducible build

### s6-overlay v3

**Usage**: Initialization system and service management.

**Reasons for choice:**
- Standard for Home Assistant Apps
- Robust process management
- Startup/shutdown script support

**Scripts:**
- `rootfs/etc/services.d/ha-box/run`: Startup
- `rootfs/etc/services.d/ha-box/finish`: Shutdown/cleanup

---

## Python Libraries

### Communication with Home Assistant

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `bashio` | Included | Supervisor API | Included in HA images |
| `requests` | Latest | HTTP client for HA API | `pip install requests` |

### Hardware - Communication Buses

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `smbus2` | Latest | I2C access | `pip install smbus2` |
| `spidev` | Latest | SPI access | `pip install spidev` |
| `RPi.GPIO` | Latest | GPIO access | `pip install RPi.GPIO` |
| `gpiozero` | Latest | Alternative GPIO (higher level) | `pip install gpiozero` |

### Hardware - Devices

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `adafruit-circuitpython-bme280` | Latest | BME280 sensor | `pip install adafruit-circuitpython-bme280` |
| `adafruit-circuitpython-pn532` | Latest | PN532 NFC module | `pip install adafruit-circuitpython-pn532` |

### Graphics

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `Pillow` | Latest | Graphic rendering, image conversion | `pip install Pillow` |

### Utilities

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `python-dateutil` | Latest | Date/time management | `pip install python-dateutil` |

---

## Alternatives Considered

### GPIO

- **RPi.GPIO**: Main choice (standard, well documented)
- **gpiozero**: Higher level alternative, can be used to simplify some cases

### E-Paper

- **waveshare-epd**: If compatible with GDEY037T03-FT21
- **Custom driver**: Probably necessary (based on UC8253 datasheet)

### NFC

- **nfcpy**: Generic alternative, but Adafruit simpler
- **libnfc**: Via Python bindings, more complex

---

## Development Tools

### Linting and Formatting

| Tool | Usage | Installation |
|------|-------|--------------|
| `pylint` | Python linting | `pip install pylint` |
| `flake8` | Python linting (alternative) | `pip install flake8` |
| `black` | Python formatting | `pip install black` |
| `shellcheck` | Bash linting | `apt-get install shellcheck` |

### Tests (optional)

| Tool | Usage | Installation |
|------|-------|--------------|
| `pytest` | Unit tests | `pip install pytest` |
| `pytest-cov` | Code coverage | `pip install pytest-cov` |

---

## Dependency Structure

### requirements.txt (to be created)

```txt
# Communication
requests>=2.31.0

# Hardware - Buses
smbus2>=0.4.3
spidev>=3.6
RPi.GPIO>=0.7.1

# Hardware - Devices
adafruit-circuitpython-bme280>=2.5.11
adafruit-circuitpython-pn532>=1.4.0

# Graphics
Pillow>=10.0.0

# Utilities
python-dateutil>=2.8.2
```

### Installation in Dockerfile

```dockerfile
RUN pip3 install --no-cache-dir -r requirements.txt
```

---

## Compatibility

### Python

- **Minimum**: Python 3.9
- **Recommended**: Python 3.11+
- **Tested on**: Python 3.9, 3.10, 3.11 (depending on HA images)

### Architecture

- **Main**: aarch64 (Raspberry Pi 4/5)
- **Future support**: amd64 (for development/testing)

### Home Assistant

- **Minimum version**: Home Assistant OS 12+
- **Supervisor API**: Compatible with recent versions

---

## Performance and Constraints

### E-Paper

- **Slow refresh**: Optimize updates
- **1-bit only**: Image conversion necessary
- **Consumption**: Very low, ideal for embedded

### I2C

- **Shared bus**: All devices on same bus
- **Speed**: 100kHz standard, 400kHz fast mode
- **Management**: Polling or interrupt depending on device

### GPIO/PWM

- **Hardware PWM**: Preferred for precision (GPIO 18)
- **Software PWM**: Alternative if hardware unavailable
- **Critical timing**: WS2812B requires precise timing

---

*Last updated: 2026-01-17*
