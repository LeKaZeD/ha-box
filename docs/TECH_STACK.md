# Technical Stack - HA Box

This document details the technologies, languages, and libraries used in the project. The **Application** (Python) runs on the Raspberry Pi and talks to the **ESP32** over UART; the ESP32 runs the display, sensors, touch, NFC, fan, and power-button logic (see [docs/ARCHITECTURE.md](ARCHITECTURE.md)).

## Languages

### Python 3.9+

**Usage (Application):** Business logic, Supervisor/Core API client, UART communication with the ESP32, configuration, i18n. No direct hardware access (no I2C, SPI, or GPIO on the Pi).

**Reasons for choice:**
- Standard in Home Assistant images
- Good library support for HTTP and serial
- Easy to maintain and debug

**Standards:**
- PEP 8 for style
- Type hints for public APIs
- Google-style docstrings
- Python 3.9+ minimum (Home Assistant compatibility)

### C++ (Arduino / ESP-IDF)

**Usage (ESP32 firmware):** Display driver (GxEPD2), touch (FT6336), BME280, NFC (PN7161 planned), fan PWM, power button, UART protocol (AsciiProto). See the `esp/ESPHOMEASSISTANT/` firmware and [docs/HARDWARE.md](HARDWARE.md).

### Bash 5.x

**Usage:** s6-overlay startup/shutdown scripts in the add-on container.

**Standards:**
- Shebang: `#!/usr/bin/with-contenv bashio`
- Indentation: 2 spaces
- Variables always quoted: `"${variable}"`

### YAML

**Usage:** Add-on configuration, build, translations.

**Files:**
- `ha-box/config.yaml`: Add-on options and schema
- `ha-box/build.yaml`: Multi-arch build (if used)
- `ha-box/translations/*.yaml`: UI translations (en, fr)

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

## Python libraries (add-on)

### Communication

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `requests` | >=2.31.0 | HTTP client for Supervisor and Core API | `pip install requests` |
| `pyserial` | >=3.5 | UART serial link to ESP32 (`/dev/serial0`, 115200 8N1) | `pip install pyserial` |

### Configuration and i18n

| Library | Version | Usage | Installation |
|---------|---------|-------|--------------|
| `PyYAML` | >=6.0 | Load translation files (`core/i18n.py`) | `pip install PyYAML` |

### Not used on the add-on

The add-on does **not** use I2C, SPI, or GPIO on the Pi. Display, BME280, touch, NFC (PN7161), and fan are on the **ESP32**. If future features require hardware on the Pi, the following could be added (and must go through a HAL if one is introduced):

- `smbus2` / `spidev` / `RPi.GPIO`: buses and GPIO
- `adafruit-circuitpython-bme280`: BME280 (currently on ESP32)
- `adafruit-circuitpython-pn7160` or equivalent: PN7161 (currently planned on ESP32)
- `Pillow`: image rendering (currently on ESP32 via GxEPD2)

---

## Alternatives considered

### UART / serial

- **pyserial**: Used for the ESP32 link; standard and well supported.
- **Alternative**: Raw `open()` on `/dev/serial0` with termios; more work for framing and timeouts.

### ESP32 firmware (E-Paper, NFC)

- **E-Paper**: GxEPD2 driver for GDEY037T03; custom panel class in firmware.
- **NFC**: PN7161 on ESP32 (I2C); driver to be integrated. Add-on only receives `NFC` messages over UART.
- **BME280**: Implemented on ESP32 (e.g. BME280Min or similar); add-on receives `SENS` and pushes to HA.

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

## Dependency structure (add-on)

### requirements.txt

The add-on uses a minimal set of dependencies (see `ha-box/requirements.txt`):

```txt
# Communication with Supervisor and Core API
requests>=2.31.0

# Configuration and i18n (translation files)
PyYAML>=6.0

# Serial communication with ESP32 (UART)
pyserial>=3.5
```

### Installation in Dockerfile

```dockerfile
RUN pip3 install --no-cache-dir -r requirements.txt
```

`bashio` is provided by the Home Assistant base image and is used by the s6 scripts, not by Python.

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

## Performance and constraints

### Add-on (Pi)

- **API calls**: Timeouts and bounded retries on all Supervisor/Core requests (see `api/ha_api.py`). Core availability is cached for a short interval to avoid excessive checks.
- **Serial**: Polling thread reads UART in a loop; line-based protocol keeps parsing simple. No blocking on single-byte reads for long periods.
- **Handlers**: Weather, clock, and status run on timers (e.g. 5 min, 1 min, 30 s); sensor updates to HA are triggered by incoming SENS messages.

### ESP32 (firmware)

- **E-Paper**: Slow refresh; updates are optimized (full vs partial, dirty regions). Display logic and rendering are entirely on the ESP32.
- **I2C**: Shared bus for touch, BME280, NFC (PN7161); addresses and timing are device-specific (see [docs/HARDWARE.md](HARDWARE.md)).
- **UART**: 115200 8N1; ASCII protocol with optional ACK for critical commands (e.g. SHUTDOWN_REQUEST, SENS).

---

*Last updated: 2026-02*
