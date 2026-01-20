<!-- https://developers.home-assistant.io/docs/add-ons/presentation#keeping-a-changelog -->

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
