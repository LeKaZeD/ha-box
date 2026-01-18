<!-- https://developers.home-assistant.io/docs/add-ons/presentation#keeping-a-changelog -->

## 0.1.0 (2026-01-17)

### Added

- Initial release
- Infrastructure de base complète
- HAL (Hardware Abstraction Layer) pour I2C, SPI, GPIO
- Client API Home Assistant
- Gestion de configuration
- Scripts s6-overlay pour démarrage/arrêt
- Structure Python modulaire
- Support configuration via options.json

### Known Issues

- Les drivers matériels (BME280, E-Paper, NFC) ne sont pas encore implémentés
- L'interface utilisateur sur l'écran E-Paper n'est pas encore développée
