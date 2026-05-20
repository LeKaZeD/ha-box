# Roadmap - HA Box

## What works today (v0.2.0)

### App (Add-on) infrastructure
- Python App running on Home Assistant OS (Docker, s6-overlay)
- Supervisor + Core API client with retry logic
- AppArmor security profile (custom, least privilege)
- Configuration via App options (`config.yaml`)
- Multilingual support (FR/EN): auto-detected from HA Core config

### ESP32 ↔ Pi communication
- ASCII UART protocol over UART0 (115200 8N1)
- Connection management: READY handshake, reconnect on timeout
- OTA firmware update: automatic version check on connect, flash via esptool over UART0, boot failure recovery (ESP32 stuck in reset loop auto-reflashed)

### ESP32 features
- E-Paper display (3.7" GDEY037T03-FT21): Loading, Home, Settings pages
- Touch input (FT6336U): navigation, page switching
- BME280: temperature, humidity, pressure — sent to Pi every 30 s
- TMP102: case temperature — sent to Pi every 30 s
- Fan (PWM, temperature-based curve, configurable from App options, persisted in NVS)
- Power button: wake Pi (J2 pulse), graceful shutdown request, hard J2 stop on long press
- Deep sleep: heartbeat timeout (5 min), shutdown fallback (60 s)

### Pi App features
- Weather + outdoor temperature on E-Paper (configurable entity, updated every 5 min)
- Clock sync to ESP32 (every 60 s)
- HA status icons (Core, Supervisor, LAN, Wi-Fi, external access, Zigbee, Thread, Matter)
- Sensor entities in HA: BME280 × 3, TMP102 case temperature
- Shutdown host on button press (Supervisor API)
- "All Off" relay (light.turn_off on HA)

---

## Planned

### Pi App
- CC1101 433 MHz *(experimental)*: SPI driver + RFLink TCP server exist; HA events/entities not yet published

### Hardware / product
- Compute Module 5 carrier board: integrate Pi, ESP32, radios, and connectors on a single PCB for a cleaner, more reliable product

---

## Known gaps / to review

| Issue | Location | Notes |
|-------|----------|-------|
| Button long press when Pi is OFF: no action | `PowerButton` | `handleLongPress()` has no else branch for Pi OFF state |
| Heartbeat timeout mismatch | `config.h` vs `.ino` | Default in `.h` is 5000 ms; overridden in `.ino` to 300 000 ms |
| Buzzer trigger points not reviewed | ESP32 firmware | 100 ms beep on event; exact triggers unverified |

---

## Dropped features

- **NFC (PN7161)**: Removed from scope. Users can use their smartphone's built-in NFC to pair new devices directly — a dedicated on-device NFC reader adds hardware cost and complexity without a clear advantage.

---

*Last updated: 2026-04-17*
