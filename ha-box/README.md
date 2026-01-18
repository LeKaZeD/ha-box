# Home Assistant Add-on: HA Box

_Interface physique pour Home Assistant avec écran E-Paper, capteurs et contrôleurs._

![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

## À propos

HA Box est un add-on Home Assistant OS qui permet de créer une interface physique pour votre installation Home Assistant via :

- 🖥️ **Écran E-Paper 3.7"** (GDEY037T03-FT21) avec front-light intégré
- 👆 **Interface tactile** (FT6336U intégré)
- 📡 **Lecteur NFC** (PN532)
- 🌡️ **Capteur environnemental** (BME280 - température, humidité, pression)
- 💡 **Bande LED** (WS2812B - optionnel)
- 🌀 **Ventilateur PWM** (optionnel)

## Installation

1. Ajoutez ce dépôt à vos add-ons Home Assistant
2. Installez l'add-on "HA Box"
3. Configurez les options selon votre matériel
4. Démarrez l'add-on

## Configuration

Consultez la documentation complète dans le dépôt principal pour la configuration détaillée.

### Prérequis matériels

- Raspberry Pi 4 ou 5
- Écran E-Paper GDEY037T03-FT21
- Capteur BME280
- Module NFC PN532 (optionnel)
- Bande LED WS2812B (optionnel)
- Ventilateur PWM 5V (optionnel)

### Configuration Raspberry Pi

Activez I2C et SPI dans `/mnt/boot/config.txt` :

```ini
dtparam=i2c_arm=on
dtparam=i2c1=on
dtparam=spi=on
```

## Support

Pour les problèmes, questions ou contributions, consultez le dépôt principal du projet.

## License

Apache License 2.0
