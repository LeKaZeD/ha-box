# Technical Architecture - HA Box

This document describes the technical architecture of the HA Box add-on.

## Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     Home Assistant OS                            │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    Supervisor                              │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │              HA Box Add-on (Container)              │  │  │
│  │  │                                                     │  │  │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐            │  │  │
│  │  │  │ Display │  │ Sensors │  │ Control │            │  │  │
│  │  │  │ Manager │  │ Manager │  │ Manager │            │  │  │
│  │  │  └────┬────┘  └────┬────┘  └────┬────┘            │  │  │
│  │  │       │            │            │                  │  │  │
│  │  │  ┌────┴────────────┴────────────┴────┐            │  │  │
│  │  │  │         Hardware Abstraction       │            │  │  │
│  │  │  │              Layer (HAL)           │            │  │  │
│  │  │  └────────────────┬───────────────────┘            │  │  │
│  │  └───────────────────┼───────────────────────────────┘  │  │
│  └──────────────────────┼──────────────────────────────────┘  │
│                         │                                      │
│  ┌──────────────────────┼──────────────────────────────────┐  │
│  │                 Linux Kernel                             │  │
│  │    /dev/spidev0.0  /dev/i2c-1  /sys/class/gpio          │  │
│  └──────────────────────┼──────────────────────────────────┘  │
└─────────────────────────┼───────────────────────────────────────┘
                          │
    ┌─────────────────────┼─────────────────────┐
    │              Raspberry Pi                  │
    │  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ │
    │  │Display│ │  NFC  │ │ Temp  │ │  LED  │ │
    │  │  SPI  │ │  I2C  │ │  I2C  │ │  PWM  │ │
    │  └───────┘ └───────┘ └───────┘ └───────┘ │
    │  ┌───────┐ ┌───────┐                     │
    │  │Touch  │ │ Fan   │                     │
    │  │  I2C  │ │  PWM  │                     │
    │  └───────┘ └───────┘                     │
    └───────────────────────────────────────────┘
```

## Main Components

### 1. Display Manager

Responsible for display on the SPI screen.

**Responsibilities:**
- Screen initialization
- Graphic rendering (text, images, icons)
- Page/screen management
- Optimized refresh

**Technologies considered:**
- Python + Pillow for rendering (1-bit conversion)
- Dedicated E-Paper library (waveshare-epd or custom driver)
- UC8253 driver based on datasheet
- Front-light management via PWM (MOSFET)

**E-Paper specifics:**
- 1-bit rendering (black/white)
- Full refresh: ~2-3 seconds
- Partial refresh: ~1 second (if supported)
- Strategy: Refresh only on significant changes
- Deep sleep mode for energy saving

### 2. Sensors Manager

Manages sensor reading.

**Responsibilities:**
- Periodic reading of I2C sensors
- Value caching
- Publication to Home Assistant
- Reading error handling

**Sensors managed:**
- BME280: Temperature, Humidity, Pressure (I2C)
- NFC: PN532 (I2C)
- Touch FT6336U: Integrated in display (I2C)

### 3. Control Manager

Manages outputs and actuators.

**Responsibilities:**
- PWM fan control
- LED strip control
- Automatic regulation
- Response to HA commands

### 4. Hardware Abstraction Layer (HAL)

Abstraction layer for hardware access.

**Responsibilities:**
- Bus abstraction (I2C, SPI, GPIO)
- Permission management
- Hardware detection
- Fallback and error handling

## Communication with Home Assistant

### Supervisor API

The add-on communicates with Home Assistant via the Supervisor API:

```
┌─────────────┐     HTTP/REST      ┌─────────────┐
│   HA Box    │ ◄────────────────► │  Supervisor │
│   Add-on    │                    │     API     │
└─────────────┘                    └──────┬──────┘
                                          │
                                   ┌──────┴──────┐
                                   │  HA Core    │
                                   │   (API)     │
                                   └─────────────┘
```

### Endpoints Used

| Endpoint | Usage |
|----------|-------|
| `/core/api/states` | Reading entity states |
| `/core/api/services` | Calling services |
| `/core/api/events` | Sending events (NFC) |
| `/addons/self/options` | Reading configuration |

### Authentication

- Supervisor token via `SUPERVISOR_TOKEN` environment variable
- Automatic access from add-on container

## File Structure

```
ha-box/
├── config.yaml           # Add-on configuration
├── build.yaml            # Build configuration
├── Dockerfile            # Docker image
├── apparmor.txt          # AppArmor profile
├── CHANGELOG.md          # Version history
├── DOCS.md               # User documentation
├── README.md             # Presentation
├── icon.png              # 256x256 icon
├── logo.png              # 256x256 logo
├── translations/
│   ├── en.yaml           # English translations
│   └── fr.yaml           # French translations
└── rootfs/
    ├── etc/
    │   └── services.d/
    │       └── ha-box/
    │           ├── run       # Startup script
    │           └── finish    # Shutdown script
    └── usr/
        └── bin/
            └── ha-box/
                ├── main.py           # Entry point
                ├── config.py         # Configuration management
                ├── display/
                │   ├── __init__.py
                │   ├── manager.py    # Display Manager
                │   ├── screens/      # Screens/pages
                │   └── drivers/      # Display drivers
                ├── sensors/
                │   ├── __init__.py
                │   ├── manager.py    # Sensors Manager
                │   ├── temperature.py
                │   ├── nfc.py
                │   └── touch.py
                ├── control/
                │   ├── __init__.py
                │   ├── manager.py    # Control Manager
                │   ├── fan.py
                │   └── led.py
                ├── hal/
                │   ├── __init__.py
                │   ├── i2c.py
                │   ├── spi.py
                │   └── gpio.py
                └── ha/
                    ├── __init__.py
                    └── client.py     # HA API client
```

## Required Hardware Configuration

### Raspberry Pi config.txt

The user must add to `/mnt/boot/config.txt`:

```ini
# Enable I2C
dtparam=i2c_arm=on
dtparam=i2c1=on

# Enable SPI
dtparam=spi=on

# PWM for fan (GPIO 18)
dtoverlay=pwm,pin=18,func=2

# SPI for E-Paper display (if necessary)
# dtoverlay=spi0-1cs
```

### Planned I2C Addresses

| Device | Address | Notes |
|--------|---------|-------|
| Touch (FT6336U) | 0x38 | Integrated in GDEY037T03-FT21 |
| NFC (PN532) | 0x24 | PN532 in I2C mode |
| BME280 | 0x76 or 0x77 | Depending on module configuration |

### GPIO Pins Used

| Pin | Function | Device | Notes |
|-----|----------|--------|-------|
| GPIO 10 (SPI MOSI) | Data | E-Paper Display | SPI 4-wire |
| GPIO 11 (SPI SCLK) | Clock | E-Paper Display | SPI |
| GPIO 8 (SPI CE0) | Chip Select | E-Paper Display | SPI |
| GPIO (TBD) | DC | E-Paper Display | Data/Command |
| GPIO (TBD) | Reset | E-Paper Display | Reset |
| GPIO (TBD) | BUSY | E-Paper Display | Status (read) |
| GPIO 2 (SDA) | I2C Data | Touch, NFC, BME280 | Shared I2C bus |
| GPIO 3 (SCL) | I2C Clock | Touch, NFC, BME280 | Shared I2C bus |
| GPIO 18 | PWM | Fan | Hardware PWM |
| GPIO 21 | Data | WS2812 LED | Optional |
| GPIO (TBD) | Front-light PWM | E-Paper Display | MOSFET front-light control (PWM) |

**Note**: The exact pins for the E-Paper display (DC, Reset, BUSY) depend on the breakout board used. To be verified during hardware integration.

## Startup Sequence

```
┌────────────────────────────────────────────────────────────────┐
│ 1. Supervisor starts the container                             │
├────────────────────────────────────────────────────────────────┤
│ 2. s6-overlay initializes services                             │
├────────────────────────────────────────────────────────────────┤
│ 3. 'run' script executes main.py                              │
├────────────────────────────────────────────────────────────────┤
│ 4. HAL: Hardware detection and initialization                  │
│    - Check I2C available                                       │
│    - Check SPI available                                       │
│    - Scan devices                                              │
├────────────────────────────────────────────────────────────────┤
│ 5. Display Manager: Display initialization                    │
│    - Display boot screen                                       │
├────────────────────────────────────────────────────────────────┤
│ 6. Sensors Manager: Start readings                             │
│    - First temperature reading                                 │
│    - NFC initialization                                        │
│    - Touch calibration                                         │
├────────────────────────────────────────────────────────────────┤
│ 7. Control Manager: Output initialization                     │
│    - PWM fan configuration                                     │
│    - LED initialization                                        │
├────────────────────────────────────────────────────────────────┤
│ 8. Connect to Home Assistant API                               │
│    - Wait if HA not ready yet                                  │
│    - Retrieve configured entities                              │
├────────────────────────────────────────────────────────────────┤
│ 9. Main loop                                                   │
│    - Update display                                            │
│    - Read sensors                                              │
│    - Process touch/NFC events                                  │
│    - Fan regulation                                            │
└────────────────────────────────────────────────────────────────┘
```

## Error Handling

### Fallback Strategy

| Error | Behavior |
|-------|----------|
| Display not detected | Log warning, headless mode |
| I2C unavailable | Log error, disable I2C sensors |
| HA not accessible | Retry with backoff, degraded display mode |
| Sensor error | Skip reading, use last value |

### Logging

Use `bashio::log.*` for bash scripts and Python `logging` module:

```python
import logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("ha-box")
```

Levels:
- `DEBUG`: Technical details
- `INFO`: Normal events
- `WARNING`: Non-blocking issues
- `ERROR`: Recoverable errors
- `CRITICAL`: Fatal errors

## Security

### Required Permissions

```yaml
# config.yaml
devices:
  - /dev/i2c-1
  - /dev/spidev0.0
  - /dev/gpiomem
gpio: true
kernel_modules: true
```

### AppArmor

Custom AppArmor profile to limit access:
- Read/write access to declared devices
- Network access limited to Supervisor API
- No access to host filesystem

---

## Multilingual Support (i18n)

HA Box supports multiple languages for the configuration interface and user messages.

### Structure

- **Translation files**: `translations/{language}.yaml` (fr, en, etc.)
- **Python module**: `ha-box/i18n.py` to load and use translations
- **Automatic detection**: Language detected from Home Assistant or environment variable

### Usage

- **Configuration**: Labels and descriptions in `config.yaml` automatically translated by HA
- **Python code**: `translator.get("common.temperature")` to retrieve translations
- **E-Paper display**: Displayed texts translated according to configured language

**See [docs/I18N.md](I18N.md) for complete details**

## Future Evolutions

- [ ] Support for multiple display types
- [ ] Plugin system for additional drivers
- [ ] Simulation mode for development without hardware
- [ ] Advanced web configuration interface
- [ ] Support for additional languages (DE, ES, etc.)

---

*Last updated: 2026-01-17*
