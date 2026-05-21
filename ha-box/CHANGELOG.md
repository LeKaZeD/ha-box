<!-- https://developers.home-assistant.io/docs/apps/presentation#keeping-a-changelog -->

## 0.2.0

### Added

- **OTA firmware update**: app automatically flashes the ESP32 when the bundled firmware version differs from the reported ESP32 version; controlled via `box.ota.io0_gpio` and `box.ota.en_gpio` config options; VERSION verb retry (up to 3 attempts), partial response salvage, and blind OTA recovery on ROM boot errors (up to 3 attempts)
- **Shutdown detection**: STATUS heartbeat accelerates to 5 s when the Pi receives a shutdown request from the ESP32; ESP32 transitions to `HALTING` state and enters deep sleep only when heartbeat silence is detected, or when the Pi 3.3V rail (GPIO5 via 1 kΩ) drops — whichever comes first
- **Pi alive hardware signal**: Pi 3.3V rail → 1 kΩ → ESP32 GPIO5 (VEN pad); gives a reliable ~10 s hardware warning before full Pi shutdown
- **CC1101 433 MHz module** *(experimental)*: SPI0/CE0 driver (no external library) exposing a RFLink-compatible TCP server on port 5557; the HA RFLink integration connects to `<pi-ip>:5557` to control any 433 MHz device; fires `ha_box_rf433_received` event on signal reception
- **Day/night mode sync**: polls `sun.sun` entity and sends `DAYMODE mode=0/1` to the ESP32 (0 = day, 1 = night); configurable via `daynight.update_interval`
- **Custom HA integration** (`ha_box`): groups all HA Box entities under a single device in the device registry
- **HA alerts widget** (E-Paper): Home page now shows warning and error counts from Home Assistant; driven by the `warn` and `err` keys in the `STATUS` verb
- **Zigbee / Thread / Matter detection**: now uses the HA config entries API for reliable detection, replacing heuristic-based checks
- **Configurable log level**: `box.log_level` option (trace / debug / info / warning / error / critical)
- **Extended config structure**: all options grouped by section: `general`, `weather`, `bme280`, `fan`, `ota`, `daynight`, `clock`, `status`, `uart`, `connection`, `rf433`

### ESP32 firmware (bundled v0.1.1)

- `PiState::HALTING`: new state between SHUTDOWN_ACCEPTED and deep sleep; waits for heartbeat silence or Pi 3.3V GPIO drop
- Pi alive GPIO (GPIO5 / VEN pad): `INPUT_PULLDOWN`; LOW = Pi off → triggers deep sleep
- HA alerts widget: displays `warn` and `err` counts received via `STATUS` verb
- 433 MHz icon (`MHZ433_ON/OFF_24x24`) shown in status bar based on `mhz433` STATUS key
- New icons: `check_circle`, `error_circle`, `warning_circle`, `update`, `433_on`, `433_off`
- WiFi and Bluetooth disabled at boot to reduce power consumption and RF interference
- Project migrated to PlatformIO (`ha-box-esp/`); dependencies managed via `platformio.ini`

---

## 0.1.45 (2026-03-31)

### Added

- Fan control configurable from App options (`esp32.fan.enabled`, `esp32.fan.min_temp`, `esp32.fan.max_temp`): values are sent to the ESP32 on every connection via the new `FAN` UART verb and persisted in ESP32 NVS so they survive deep sleep
- Case temperature sensor: ESP32 forwards TMP102 readings to the App every 30 s via the new `CASE` UART verb; exposed in Home Assistant as `sensor.ha_box_case_temperature`

### Changed

- Config section `control` renamed to `esp32` to clearly identify options that are pushed to the ESP32 firmware
- `esp32.fan`: removed unused `pin` and `auto_control` fields; default `min_temp` aligned to 28 °C (matches ESP32 firmware default)
- External access detection: Cloudflare tunnel Apps now trust the Supervisor running state instead of attempting an outbound HTTP check (which failed inside the container with Zero Trust enabled)
- Added Cloudflare App slug `9b69fd20_cloudflared` (brenner-tobias) to the default exposed App list

---
<!-- Versions 0.1.1 – 0.1.44 not yet documented. -->
---

## 0.1.0 (2026-01-20)

### Added

- Initial release
- Complete base infrastructure
- HAL (Hardware Abstraction Layer) for I2C, SPI, GPIO
- Home Assistant API client with retry logic
- Configuration management
- s6-overlay scripts for startup/shutdown
- Modular Python structure
- Configuration support via options.json
- Custom AppArmor security profile
- Security documentation (SECURITY.md)
- Presentation assets (icon.png, logo.png)
- Development setup for amd64 (gitignored)
- Optimized Dockerfile (build dependencies cleanup)
- Multilingual support infrastructure (translations/ folder, i18n.py module)
- Translation files for English and French (config.yaml labels/descriptions)

### Security

- Security rating: 6/6 (maximum)
- Custom AppArmor profile restricting to hardware devices only
- No privileged mode, no host network
- Minimal permissions following least privilege principle

### Known Issues

- Hardware drivers (BME280, E-Paper, NFC) are not yet implemented
- User interface on E-Paper display is not yet developed
