<!-- https://developers.home-assistant.io/docs/apps/presentation#keeping-a-changelog -->

## 0.1.45 (2026-03-31)

### Added

- Fan control configurable from add-on options (`esp32.fan.enabled`, `esp32.fan.min_temp`, `esp32.fan.max_temp`): values are sent to the ESP32 on every connection via the new `FAN` UART verb and persisted in ESP32 NVS so they survive deep sleep
- Case temperature sensor: ESP32 forwards TMP102 readings to the add-on every 30 s via the new `CASE` UART verb; exposed in Home Assistant as `sensor.ha_box_case_temperature`

### Changed

- Config section `control` renamed to `esp32` to clearly identify options that are pushed to the ESP32 firmware
- `esp32.fan`: removed unused `pin` and `auto_control` fields; default `min_temp` aligned to 28 °C (matches ESP32 firmware default)
- External access detection: Cloudflare tunnel add-ons now trust the Supervisor running state instead of attempting an outbound HTTP check (which failed inside the container with Zero Trust enabled)
- Added Cloudflare add-on slug `9b69fd20_cloudflared` (brenner-tobias) to the default exposed add-on list

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
