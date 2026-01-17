# HA Box - Add-on Home Assistant pour Raspberry Pi

## Vision du projet

**HA Box** est un add-on Home Assistant OS pour Raspberry Pi permettant de contrôler un ensemble de périphériques matériels :

- 🖥️ **Écran SPI** - Affichage d'informations Home Assistant
- 👆 **Tactile I2C** - Interface de navigation
- 📡 **Capteur NFC I2C** - Lecture de tags NFC
- 🌡️ **Capteur de température I2C** - Mesure de température ambiante
- 💡 **Bande LED** - Effets visuels et notifications
- 🌀 **Ventilateur PWM** - Régulation thermique

## Objectif principal

Fournir une interface physique simple et élégante pour afficher et interagir avec certaines informations de Home Assistant, directement sur un écran connecté au Raspberry Pi.

## Contraintes techniques identifiées

### Ordre de démarrage

L'add-on peut utiliser le paramètre `startup` dans `config.yaml` :
- `initialize` : Démarre très tôt, avant les autres services
- `system` : Démarre avec les services système
- `services` : Démarre après les services système
- `application` : Démarre après Home Assistant (défaut)

⚠️ **Important** : Les add-ons sont gérés par le Supervisor, qui démarre après le boot de l'OS. Un add-on ne peut pas démarrer avant le système lui-même.

### Accès matériel

Pour accéder aux bus SPI/I2C/GPIO, il faut :
1. Activer les interfaces dans `config.txt` du Raspberry Pi
2. Déclarer les périphériques dans le `config.yaml` de l'add-on
3. Potentiellement désactiver le "Protection Mode"

### Add-ons existants de référence

- [ha-rpi_gpio](https://github.com/thecode/ha-rpi_gpio) - Accès GPIO
- [Pironman](https://github.com/sunfounder/home-assistant-addon) - Gestion écran/LED/ventilo
- [HassOS I2C Configurator](https://community.home-assistant.io/t/add-on-hassos-i2c-configurator/264167) - Configuration I2C

## Statut du projet

🚧 **En cours de définition** - Phase de conception et documentation

---

*Dernière mise à jour : 2026-01-17*
