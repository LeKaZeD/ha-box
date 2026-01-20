# Roadmap - HA Box

This document describes the current state of the project and the next development steps.

## Current Project Status

### Phase 1: Design and Documentation (COMPLETED)

| Task | Status | Notes |
|------|--------|-------|
| Project documentation | Completed | `docs/PROJECT.md` |
| Features specification | Completed | `docs/FEATURES.md` (8 features defined) |
| Technical architecture | Completed | `docs/ARCHITECTURE.md` |
| Hardware specifications | Completed | `docs/HARDWARE.md` |
| Contribution guide | Completed | `CONTRIBUTING.md` |
| Development rules | Completed | `.cursorrules` |
| Main README | Completed | `README.md` |

**Result**: Complete documentation, hardware identified, architecture defined.

---

## Technical Stack

### Languages and Technologies

| Component | Technology | Version | Usage |
|-----------|------------|---------|-------|
| **Container** | Docker | Latest | Based on Home Assistant images |
| **Init system** | s6-overlay | v3 | Service management |
| **System scripts** | Bash | 5.x | Startup/shutdown scripts |
| **Main application** | Python | 3.9+ | Business application |
| **Configuration** | YAML | - | Add-on config, build |
| **Build** | Docker Buildx | - | Multi-arch build (aarch64) |

### Planned Python Libraries

| Library | Usage | Installation |
|---------|-------|--------------|
| `bashio` | Supervisor API access | Included in HA images |
| `requests` | HTTP communication with HA | `pip install requests` |
| `Pillow` | E-Paper graphic rendering | `pip install Pillow` |
| `adafruit-circuitpython-bme280` | BME280 sensor | `pip install adafruit-circuitpython-bme280` |
| `adafruit-circuitpython-pn532` | PN532 NFC module | `pip install adafruit-circuitpython-pn532` |
| `RPi.GPIO` or `gpiozero` | GPIO access | `pip install RPi.GPIO` |
| `spidev` | SPI access | `pip install spidev` |
| `smbus2` | I2C access | `pip install smbus2` |

### Development Tools

- **Linting**: `pylint`, `flake8`, `shellcheck`
- **Formatting**: `black` (Python), automatic Bash formatting
- **Tests**: `pytest` (if unit tests added)
- **Versioning**: Git with Gitflow

---

## Development Phases

### Phase 2: Base Infrastructure (COMPLETED)

**Objective**: Create the base structure of the add-on and minimal infrastructure.

| Task | Priority | Status | Notes |
|------|----------|--------|-------|
| Create `ha-box/` structure | Critical | Completed | Base structure created |
| Configure `config.yaml` | Critical | Completed | Permissions, devices, options configured |
| Configure `build.yaml` | Critical | Completed | Multi-arch build configured |
| Create `Dockerfile` | Critical | Completed | Image with Python dependencies |
| s6 scripts (`run`, `finish`) | Critical | Completed | Startup/shutdown implemented |
| Base Python structure | Critical | Completed | `main.py`, base modules created |
| HAL (Hardware Abstraction Layer) | High | Completed | `hal/i2c.py`, `hal/spi.py`, `hal/gpio.py` |
| Home Assistant API client | High | Completed | `ha/client.py` implemented |
| Configuration management | High | Completed | `config.py` with options loading |
| Logging and error handling | High | Completed | Logging infrastructure created |

**Actual duration**: Completed

**Result**: Complete infrastructure created, ready for Phase 3 (hardware support)

#### 2.1 Multilingual Support (Medium priority)

| Task | Status | Notes |
|------|--------|-------|
| Create `translations/` structure | To do | YAML files by language (fr, en) |
| Implement `i18n.py` module | To do | YAML loading, language detection |
| Translate configuration interface | To do | Labels/descriptions in config.yaml |
| Translate E-Paper screen texts | To do | Phase 5 - UI messages |

**Note**: YAML translation files are self-documenting by their structure. See `docs/I18N.md` for general operation.

---

### Phase 3: Base Hardware Support (IN PROGRESS)

**Objective**: Implement support for essential hardware devices.

#### 3.1 BME280 Sensor (High priority)

| Task | Status | Notes |
|------|--------|-------|
| BME280 driver | To do | Temperature, humidity, pressure reading |
| Automatic I2C address detection | To do | 0x76 or 0x77 |
| Exposure as HA sensors | To do | 3 HA entities |
| Error handling | To do | Timeout, disconnection |

#### 3.2 E-Paper Display (High priority)

| Task | Status | Notes |
|------|--------|-------|
| UC8253 driver | To do | SPI communication, initialization |
| Graphic rendering | To do | Image → 1-bit conversion |
| Refresh management | To do | Full/partial, optimization |
| Front-light PWM | To do | MOSFET control via PWM |
| Boot screen | To do | Display at startup |

#### 3.3 FT6336U Touch (Medium priority)

| Task | Status | Notes |
|------|--------|-------|
| FT6336U driver | To do | I2C touch reading |
| Calibration | To do | 240×416 mapping |
| Gesture handling | To do | Tap, long press, swipe |

**Estimated duration**: 3-4 weeks

---

### Phase 4: Advanced Features

**Objective**: Add complementary features.

#### 4.1 NFC PN532

| Task | Status | Notes |
|------|--------|-------|
| PN532 driver | To do | I2C communication |
| Tag detection | To do | Continuous polling |
| HA integration | To do | HA events |
| Protocol support | To do | MIFARE, NTAG |

#### 4.2 PWM Fan

| Task | Status | Notes |
|------|--------|-------|
| PWM control | To do | GPIO 18, hardware PWM |
| Automatic regulation | To do | Based on temperature |
| Exposure as HA fan | To do | Fan entity |

#### 4.3 LED Strip (Optional)

| Task | Status | Notes |
|------|--------|-------|
| WS2812B driver | To do | Critical timing |
| Effects | To do | Fixed, fade, rainbow |
| Exposure as HA light | To do | Light entity |

**Estimated duration**: 2-3 weeks

---

### Phase 5: User Interface and Integration

**Objective**: Create the user interface on the E-Paper display and finalize multilingual support.

| Task | Status | Notes |
|------|--------|-------|
| Screen system | To do | Multiple pages |
| Main screen | To do | HA dashboard |
| Touch navigation | To do | Swipe between screens |
| HA entity display | To do | Configurable selection |
| Widgets | To do | Temperature, clock, etc. |
| Configuration UI | To do | Options in add-on |
| UI translation integration | To do | Texts displayed on E-Paper screen |

**Estimated duration**: 2-3 weeks

---

### Phase 6: Testing and Optimization

**Objective**: Test on real hardware and optimize.

| Task | Status | Notes |
|------|--------|-------|
| Real hardware tests | To do | Raspberry Pi 4/5 |
| Integration tests | To do | All devices |
| Performance optimization | To do | E-Paper refresh |
| Robust error handling | To do | Fallbacks, retry |
| User documentation | To do | Installation guide |
| Load tests | To do | Long-term stability |

**Estimated duration**: 1-2 weeks

---

### Phase 7: Release

**Objective**: Prepare the first release.

| Task | Status | Notes |
|------|--------|-------|
| Version 0.1.0 alpha | To do | Test version |
| Version 1.0.0 | To do | Stable version |
| Complete documentation | To do | README, guides |
| CI/CD | To do | Automatic build |
| Changelog | To do | Version history |

**Estimated duration**: 1 week

---

## Estimated Timeline

| Phase | Duration | Dependencies |
|-------|----------|--------------|
| Phase 1: Documentation | Completed | - |
| Phase 2: Infrastructure | Completed | Phase 1 |
| Phase 2.1: Multilingual support | 2-3 days | Phase 2 |
| Phase 3: Base hardware | 3-4 weeks | Phase 2 |
| Phase 4: Advanced features | 2-3 weeks | Phase 3 |
| Phase 5: User interface | 2-3 weeks | Phase 3, Phase 2.1 |
| Phase 6: Testing | 1-2 weeks | Phase 4, 5 |
| Phase 7: Release | 1 week | Phase 6 |

**Total estimated**: 10-15 weeks (2.5-4 months)

**Note**: Multilingual support (Phase 2.1) can be done in parallel with Phase 3.

---

## Immediate Next Actions

1. **Rename `example/` to `ha-box/`** and adapt structure
2. **Configure `config.yaml`** with hardware permissions
3. **Create `Dockerfile`** with Python dependencies
4. **Implement HAL** (Hardware Abstraction Layer) base
5. **Create `main.py`** with main loop

---

## Important Notes

- **Required hardware**: Test on real Raspberry Pi as soon as possible
- **E-Paper**: Slow refresh requires adapted interface
- **Priorities**: BME280 and E-Paper Display first (critical features)
- **Tests**: Test each component individually before integration

---

*Last updated: 2026-01-17*
