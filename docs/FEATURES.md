# Features Specification - HA Box

This document lists all planned features, their status and specifications.

## Status Legend

| Status | Description |
|--------|-------------|
| To be defined | Needs specifications |
| Planned | Specified, awaiting development |
| In progress | Active development |
| Completed | Implemented and tested |
| Abandoned | Not retained |

## Priority Legend

| Priority | Description |
|----------|-------------|
| Critical | Blocking for the project |
| High | Main functionality |
| Medium | Important improvement |
| Low | Nice to have |

---

## F001 - E-Paper SPI Display

| Attribute | Value |
|----------|-------|
| **Priority** | High |
| **Status** | Planned |
| **Interface** | SPI 4-wire or 3-wire (`/dev/spidev0.0`) |
| **Hardware** | GDEY037T03-FT21 (GooDisplay) |

### Description

Graphic display on a 3.7" E-Paper display with integrated front-light. The display is bi-stable (retains image without power) and allows displaying:
- Home Assistant information (states, sensors)
- Time and date
- Custom messages
- Simple icons and graphics (grayscale)

**E-Paper Advantages:**
- Ultra-low consumption (34µA standby, 1.1µA deep sleep)
- Perfect readability in ambient light
- No constant refresh necessary
- Integrated front-light for use in darkness

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Model** | GDEY037T03-FT21 |
| **Size** | 3.7" |
| **Resolution** | 240×416 pixels |
| **DPI** | 130 |
| **Controller** | UC8253 |
| **Interface** | SPI 4-wire or 3-wire |
| **Front-light** | 9 LEDs, 2.8V (typical) |
| **Temperature** | -25°C to 70°C |
| **Pixels** | 1-bit (black/white) |

**Libraries considered:**
- `waveshare-epd` (if compatible)
- `epdlib` or generic E-Paper library
- Custom driver based on datasheet

**Refresh:**
- Full refresh: ~2-3 seconds
- Partial refresh: ~1 second (if supported)
- Strategy: Refresh only when necessary (data changes)

### Acceptance Criteria

- [ ] Display initializes at App startup
- [ ] Readable text display (grayscale)
- [ ] Display at least 3 Home Assistant entities
- [ ] Automatic value updates (optimized refresh)
- [ ] Front-light control (on/off, intensity)
- [ ] Deep sleep management for energy saving

### Dependencies

- SPI configuration enabled on Pi
- Access to `/dev/spidev0.0` device
- Control pins (DC, Reset, BUSY) via GPIO

### Technical Notes

- UC8253 controller requires specific initialization sequence
- BUSY signal to monitor for synchronization
- Waveform stored in OTP or loaded by MCU
- Portrait and landscape mode support

---

## F002 - I2C Touch Interface

| Attribute | Value |
|----------|-------|
| **Priority** | Medium |
| **Status** | Planned |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Hardware** | FT6336U (integrated in GDEY037T03-FT21) |

### Description

Touch input management via the FT6336U controller integrated in the E-Paper display. Allows simple interaction with the interface:
- Navigation between screens
- Element selection
- Quick actions (toggle, slider)
- **Note**: Visual feedback will be limited by E-Paper refresh speed

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Controller** | FT6336U |
| **Interface** | I2C |
| **Voltage** | 3.0V |
| **Screen Resolution** | 240×416 pixels (touch mapping) |

**Libraries considered:**
- `ft6336` (Python driver)
- Driver based on FT6336U datasheet

**Gesture management:**
- Simple tap
- Long press
- Swipe (limited by E-Paper refresh)

### Acceptance Criteria

- [ ] Touch detection
- [ ] Acceptable accuracy (±5px)
- [ ] Response < 100ms (I2C read)
- [ ] At least 3 gestures supported (tap, long press, swipe)
- [ ] Correct mapping with 240×416 resolution

### Dependencies

- F001 (E-Paper Display) - touch is integrated
- I2C configuration enabled
- FT6336U I2C address (to verify in datasheet)

### Technical Notes

- FT6336U is integrated in the module, no separate component needed
- Typical I2C address: 0x38 (to confirm)
- Multi-touch support (2 simultaneous points)

---

## F003 - PN7161 NFC Sensor

| Attribute | Value |
|----------|-------|
| **Priority** | Medium |
| **Status** | Planned (ESP32 firmware; add-on receives `NFC` over UART) |
| **Interface** | I2C on ESP32 |
| **Hardware** | PN7161 (NXP) |

### Description

NFC tag reading via PN7161 module on the ESP32. The ESP32 sends `NFC` messages (UID, and later NDEF) over UART; the add-on can trigger automations or onboarding flows in Home Assistant.

- Tag identification (UID)
- Automation triggering
- Simple authentication / onboarding
- Support for multiple NFC protocols

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Model** | PN7161 |
| **Interface** | I2C |
| **I2C Address** | 0x24 or 0x48 (configurable in add-on options) |
| **Protocols** | MIFARE Classic, NTAG, ISO14443 Type A/B, etc. |
| **Range** | ~5cm |
| **Voltage** | 3.3V |

**Implementation:** Driver and polling on **ESP32** (I2C). Add-on only receives `NFC` verb over UART; no Python NFC library on the Pi.

**Reading mode (ESP32):**
- Polling or interrupt; scan frequency configurable (e.g. 1–2 Hz).

### Acceptance Criteria

- [ ] MIFARE Classic tag reading (ESP32)
- [ ] NTAG / ISO14443 Type A tag reading (ESP32)
- [ ] ESP32 sends `NFC` message with UID to add-on
- [ ] Add-on forwards event to Home Assistant or triggers automation
- [ ] Reading time < 500ms
- [ ] Optional: NDEF payload in `NFC` message

### Dependencies

- ESP32 I2C configured for PN7161 (address 0x24 or 0x48)
- UART link add-on <-> ESP32 (see [ESP32_RASP_COM.md](ESP32_RASP_COM.md))

### Technical Notes

- PN7161 is an NXP NFC controller (successor to PN532); I2C address 0x24 or 0x48 depending on module.
- Hardware is on the **ESP32**; the add-on does not access I2C for NFC.

---

## F004 - BME280 Sensor (Temperature/Humidity/Pressure)

| Attribute | Value |
|----------|-------|
| **Priority** | High |
| **Status** | Planned |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Hardware** | BME280 (Bosch) |

### Description

Measurement of temperature, humidity and atmospheric pressure via BME280 sensor for:
- Display on E-Paper screen
- Exposure as Home Assistant entities (3 sensors)
- Fan regulation (F006) based on temperature
- Ambient condition monitoring

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Model** | BME280 |
| **Interface** | I2C (or SPI, but I2C chosen) |
| **I2C Addresses** | 0x76 or 0x77 (depending on configuration) |
| **Temperature** | -40°C to +85°C |
| **Temperature Accuracy** | ±1°C |
| **Humidity** | 0-100% RH |
| **Humidity Accuracy** | ±3% RH |
| **Pressure** | 300-1100 hPa |
| **Pressure Accuracy** | ±1 hPa |

**Libraries considered:**
- `adafruit-circuitpython-bme280` (Adafruit)
- `bme280` (standard Python driver)
- `RPi.bme280` (Raspberry Pi specific)

**Measurement frequency:**
- Reading every 30 seconds (configurable)
- Value caching to avoid I2C overload
- Sleep mode between readings for energy saving

### Acceptance Criteria

- [ ] Temperature reading with ±1°C accuracy
- [ ] Humidity reading with ±3% RH accuracy
- [ ] Pressure reading with ±1 hPa accuracy
- [ ] Exposure as 3 sensors in HA (`sensor.ha_box_temperature`, `sensor.ha_box_humidity`, `sensor.ha_box_pressure`)
- [ ] Update every 30s minimum
- [ ] Local display (formatted values)
- [ ] Automatic I2C address detection (0x76 or 0x77)

### Dependencies

- I2C configuration enabled
- F001 (optional, for display)
- I2C pull-ups (generally present on BME280 modules)

### Technical Notes

- BME280 requires initial calibration (compensation)
- Support for forced mode (on-demand measurement) or normal mode (continuous measurement)
- Configurable filter to smooth values

---

## F005 - LED Strip

| Attribute | Value |
|----------|-------|
| **Priority** | Low |
| **Status** | To be defined |
| **Interface** | GPIO/PWM or SPI |
| **Hardware** | WS2812B, SK6812, APA102, etc. |

### Description

LED strip control for:
- Visual notifications
- Ambient lighting
- Status indicator

### Technical Specifications

- [ ] LED type (WS2812B recommended)
- [ ] Maximum number of LEDs
- [ ] Control method (PWM, SPI, DMA)
- [ ] Available effects

### Acceptance Criteria

- [ ] RGB color control
- [ ] At least 3 effects (fixed, fade, rainbow)
- [ ] Integration as light in HA
- [ ] Responsiveness < 50ms

### Dependencies

- GPIO or SPI access
- Sufficient power supply

---

## F006 - PWM Fan

| Attribute | Value |
|----------|-------|
| **Priority** | Medium |
| **Status** | To be defined |
| **Interface** | GPIO PWM |
| **Hardware** | 5V PWM 4-pin fan |

### Description

Fan speed regulation based on:
- CPU temperature
- Ambient temperature (F004)
- Manual control

### Technical Specifications

- [ ] GPIO PWM pin to use
- [ ] PWM frequency
- [ ] Regulation curve
- [ ] Temperature thresholds

### Acceptance Criteria

- [ ] Speed control 0-100%
- [ ] Automatic regulation based on temperature
- [ ] Manual mode available
- [ ] Exposure as fan in HA

### Dependencies

- GPIO PWM access
- F004 (for automatic regulation)

---

## F007 - Configuration and UI

| Attribute | Value |
|----------|-------|
| **Priority** | High |
| **Status** | To be defined |
| **Interface** | Home Assistant |

### Description

Add-on configuration interface:
- Options in App panel
- Entity selection to display
- Threshold and parameter configuration

### Technical Specifications

- [ ] Configuration schema
- [ ] Input validation
- [ ] Hot reload
- [ ] Translations (FR, EN)

### Acceptance Criteria

- [ ] Functional configuration via UI
- [ ] Error validation
- [ ] Option documentation
- [ ] At least 2 languages

### Dependencies

- Basic App structure

---

## F008 - Early Startup

| Attribute | Value |
|----------|-------|
| **Priority** | Low |
| **Status** | To be defined |
| **Interface** | Supervisor |

### Description

Allow App to start as early as possible for:
- Display boot screen
- Quick device initialization
- Display HA startup status

### Technical Specifications

- [ ] `startup` value in config.yaml
- [ ] HA unavailability handling
- [ ] Fallback screen

### Acceptance Criteria

- [ ] Screen displays something from App boot
- [ ] No crash if HA not ready yet
- [ ] Smooth transition to main screen

### Dependencies

- F001 (SPI Display)
- Understanding Supervisor boot cycle

---

## F008 - Display Front-light

| Attribute | Value |
|----------|-------|
| **Priority** | Medium |
| **Status** | Planned |
| **Interface** | GPIO PWM |
| **Hardware** | 9 integrated LEDs (2.8V) controlled by MOSFET |

### Description

Control of the front-light integrated in the E-Paper display to allow reading in darkness. The front-light is controlled via a MOSFET that blocks current by default, allowing PWM control to adjust brightness:
- Activation/deactivation
- Intensity adjustment via PWM (0-100%)
- Automatic mode based on ambient light (if sensor available)
- Energy saving (automatic deactivation)

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Control** | MOSFET (PWM) |
| **LED Voltage** | 2.8V typical |
| **Number of LEDs** | 9 |
| **Max Consumption** | ~20-30mA |
| **Interface** | GPIO PWM (Hardware or Software PWM) |

**PWM Control:**
- PWM frequency: 1-10 kHz (optimize to avoid flicker)
- Resolution: 8-12 bits (256-4096 levels)
- Duty cycle: 0-100% (0% = off, 100% = max)

**Libraries considered:**
- `RPi.GPIO` with software PWM
- Raspberry Pi hardware PWM (if pin available)
- `pigpio` for more precise PWM

### Acceptance Criteria

- [ ] Functional on/off control
- [ ] Intensity adjustable via PWM (0-100%)
- [ ] No visible flicker
- [ ] Integration in App configuration
- [ ] Automatic mode (on/off based on time or light)

### Dependencies

- F001 (E-Paper Display)
- Available GPIO pin for PWM
- PWM configuration enabled

### Technical Notes

- MOSFET blocks current by default (low state = off)
- PWM allows smooth intensity control
- Avoid frequencies too low (< 100Hz) to prevent flicker
- Hardware PWM recommended if available (more precise, less CPU load)

---

## E-Paper Specifics

### Constraints and Opportunities

Using an E-Paper display brings constraints but also unique advantages:

**Constraints:**
- **Slow refresh**: 2-3 seconds for full refresh
- **1-bit display**: Black and white only, no colors
- **Ghosting**: Possible traces of previous images (requires periodic refresh)
- **Temperature**: Performance degraded below 0°C

**Advantages:**
- **Ultra-low consumption**: 34µA standby, 1.1µA deep sleep
- **Perfect readability**: Excellent contrast in ambient light
- **Bi-stable**: Image remains displayed without power
- **No glare**: Comfortable for prolonged reading
- **Ideal for static display**: Perfect for Home Assistant dashboard

### Optimization Strategies

1. **Smart refresh**:
   - Refresh only modified areas (if supported)
   - Periodic full refresh to avoid ghosting
   - Detect significant changes before refresh

2. **Adapted user interface**:
   - Minimalist design, optimized for black/white
   - Use strong contrasts
   - Avoid fast animations
   - Tactile/haptic feedback to compensate for visual latency

3. **Energy management**:
   - Deep sleep mode when display not used
   - Front-light deactivation when not needed
   - Refresh only on important changes

---

## Backlog / Future Ideas

These features are not planned but could be added:

| ID | Feature | Description |
|----|---------|-------------|
| F009 | Physical buttons | GPIO button support in addition to touch |
| F010 | Audio | Audio output for sound notifications |
| F011 | Themes | Interface customization (grayscale via dithering) |
| F012 | Widgets | Customizable widgets on screen |
| F013 | Multi-display | Multiple display support |
| F014 | Partial refresh | Optimization with E-Paper partial refresh |
| F015 | Energy saving mode | Inactivity detection and automatic deep sleep |

---

## Modification History

| Date | Modification |
|------|--------------|
| 2026-01-17 | Initial features specification creation |

---

*This document is living and will be updated as the project progresses.*
