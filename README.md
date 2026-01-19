# HA Box - Add-on Home Assistant pour Raspberry Pi

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

**HA Box** est un add-on Home Assistant OS pour Raspberry Pi permettant de contrôler un ensemble de périphériques matériels pour créer une interface physique avec Home Assistant.

## 🎯 Objectif

Afficher et interagir avec les informations de Home Assistant via un écran tactile connecté au Raspberry Pi, tout en gérant des capteurs et actuateurs locaux.

## ✨ Fonctionnalités

| Fonctionnalité | Interface | Matériel | Statut |
|----------------|-----------|----------|--------|
| 🖥️ Écran E-Paper | SPI | GDEY037T03-FT21 | 🔜 Planifié |
| 👆 Interface tactile | I2C | FT6336U (intégré) | 🔜 Planifié |
| 📡 Lecteur NFC | I2C | PN532 | 🔜 Planifié |
| 🌡️ Capteur BME280 | I2C | Température/Humidité/Pression | 🔜 Planifié |
| 💡 Bande LED | GPIO | WS2812B | 📋 À définir |
| 🌀 Ventilateur | PWM | 5V PWM | 📋 À définir |

## 📋 Prérequis

### Matériel

- Raspberry Pi 4 ou 5
- Home Assistant OS installé
- **Écran E-Paper** : GDEY037T03-FT21 (3.7", 240×416, tactile intégré)
- **Capteur environnemental** : BME280 (température, humidité, pression)
- **Module NFC** : PN532 (I2C)
- Bande LED WS2812B (optionnel)
- Ventilateur PWM 5V (optionnel)

📖 **Voir [docs/HARDWARE.md](docs/HARDWARE.md) pour les spécifications détaillées**

### Configuration du Raspberry Pi

Avant d'installer l'add-on, activez I2C et SPI dans `/mnt/boot/config.txt` :

```ini
dtparam=i2c_arm=on
dtparam=i2c1=on
dtparam=spi=on
```

## 🚀 Installation

1. Ajoutez ce dépôt à vos add-ons Home Assistant :

   [![Ajouter le dépôt](https://my.home-assistant.io/badges/supervisor_add_addon_repository.svg)](https://my.home-assistant.io/redirect/supervisor_add_addon_repository/?repository_url=https%3A%2F%2Fgithub.com%2FVOTRE_USERNAME%2Fha-box)

   Ou manuellement : **Paramètres** → **Modules complémentaires** → **Boutique** → **⋮** → **Dépôts** → Ajouter l'URL du dépôt

2. Installez l'add-on "HA Box"
3. Configurez les options selon votre matériel
4. Démarrez l'add-on

## ⚙️ Configuration

```yaml
# Exemple de configuration (à venir)
display:
  type: "ili9341"
  rotation: 0
  
sensors:
  temperature:
    enabled: true
    address: 0x76
  nfc:
    enabled: true
    address: 0x24

entities:
  - sensor.temperature_salon
  - sensor.humidity_salon
  - switch.lumiere_salon
```

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [ROADMAP.md](ROADMAP.md) | **Feuille de route** - État actuel et prochaines étapes |
| [PROJECT.md](docs/PROJECT.md) | Vision et objectifs du projet |
| [FEATURES.md](docs/FEATURES.md) | Cahier des fonctionnalités |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Architecture technique |
| [HARDWARE.md](docs/HARDWARE.md) | Spécifications matérielles détaillées |
| [TECH_STACK.md](docs/TECH_STACK.md) | Stack technique détaillée |
| [I18N.md](docs/I18N.md) | Support multilingue (i18n) |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Guide de contribution |

## 🤝 Contribuer

Les contributions sont les bienvenues ! Consultez le [guide de contribution](CONTRIBUTING.md) pour commencer.

### Comment participer

1. 📖 Lisez la documentation dans `docs/`
2. 🐛 Signalez les bugs via les Issues
3. 💡 Proposez des fonctionnalités via les Issues
4. 🔧 Soumettez des Pull Requests

## 📊 Statut du projet

🚧 **Phase de conception** - Le projet est en cours de définition. La documentation et l'architecture sont en place, le développement va bientôt commencer.

### Roadmap

- [x] Documentation initiale
- [x] Architecture technique
- [x] Cahier des fonctionnalités
- [ ] Prototype écran SPI
- [ ] Intégration capteurs I2C
- [ ] Interface tactile
- [ ] Première release alpha

## 🔗 Ressources utiles

- [Documentation Add-ons Home Assistant](https://developers.home-assistant.io/docs/add-ons)
- [ha-rpi_gpio](https://github.com/thecode/ha-rpi_gpio) - Add-on GPIO de référence
- [Pironman](https://github.com/sunfounder/home-assistant-addon) - Add-on similaire de SunFounder

## 📄 Licence

Ce projet est sous licence [Apache 2.0](LICENSE).

---

*Projet démarré le 17 janvier 2026*
