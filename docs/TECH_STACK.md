# Stack technique - HA Box

Ce document détaille toutes les technologies, langages et bibliothèques utilisés dans le projet.

## Langages

### Python 3.9+

**Usage principal** : Application métier, drivers matériels, communication avec Home Assistant.

**Raisons du choix :**
- Excellente support des bibliothèques matériel (GPIO, I2C, SPI)
- Bibliothèques Adafruit disponibles
- Facile à maintenir et déboguer
- Support natif dans les images Home Assistant

**Standards :**
- PEP 8 pour le style
- Type hints obligatoires
- Docstrings Google style
- Python 3.9+ minimum (compatibilité Home Assistant)

### Bash 5.x

**Usage** : Scripts de démarrage/arrêt s6-overlay.

**Raisons du choix :**
- Standard pour les scripts système Linux
- Intégration native avec s6-overlay
- Accès à bashio pour l'API Supervisor

**Standards :**
- Shebang : `#!/usr/bin/with-contenv bashio`
- Indentation : 2 espaces
- Variables toujours quotées : `"${variable}"`

### YAML

**Usage** : Configuration de l'add-on, build, traductions.

**Fichiers :**
- `config.yaml` : Configuration de l'add-on
- `build.yaml` : Configuration de build multi-arch
- `translations/*.yaml` : Traductions

---

## Technologies de base

### Docker

**Usage** : Conteneurisation de l'add-on.

**Image de base** : Images Home Assistant officielles
- `ghcr.io/home-assistant/{arch}-base:3.15`
- Support aarch64 (Raspberry Pi)

**Raisons du choix :**
- Standard pour les add-ons Home Assistant
- Isolation et sécurité
- Build reproductible

### s6-overlay v3

**Usage** : Système d'initialisation et gestion des services.

**Raisons du choix :**
- Standard pour les add-ons Home Assistant
- Gestion robuste des processus
- Support des scripts de démarrage/arrêt

**Scripts :**
- `rootfs/etc/services.d/ha-box/run` : Démarrage
- `rootfs/etc/services.d/ha-box/finish` : Arrêt/cleanup

---

## Bibliothèques Python

### Communication avec Home Assistant

| Bibliothèque | Version | Usage | Installation |
|--------------|---------|-------|--------------|
| `bashio` | Inclus | API Supervisor | Inclus dans images HA |
| `requests` | Latest | HTTP client pour API HA | `pip install requests` |

### Matériel - Bus de communication

| Bibliothèque | Version | Usage | Installation |
|--------------|---------|-------|--------------|
| `smbus2` | Latest | Accès I2C | `pip install smbus2` |
| `spidev` | Latest | Accès SPI | `pip install spidev` |
| `RPi.GPIO` | Latest | Accès GPIO | `pip install RPi.GPIO` |
| `gpiozero` | Latest | Alternative GPIO (plus haut niveau) | `pip install gpiozero` |

### Matériel - Périphériques

| Bibliothèque | Version | Usage | Installation |
|--------------|---------|-------|--------------|
| `adafruit-circuitpython-bme280` | Latest | Capteur BME280 | `pip install adafruit-circuitpython-bme280` |
| `adafruit-circuitpython-pn532` | Latest | Module NFC PN532 | `pip install adafruit-circuitpython-pn532` |

### Graphisme

| Bibliothèque | Version | Usage | Installation |
|--------------|---------|-------|--------------|
| `Pillow` | Latest | Rendu graphique, conversion images | `pip install Pillow` |

### Utilitaires

| Bibliothèque | Version | Usage | Installation |
|--------------|---------|-------|--------------|
| `python-dateutil` | Latest | Gestion dates/heures | `pip install python-dateutil` |

---

## Alternatives considérées

### GPIO

- **RPi.GPIO** : Choix principal (standard, bien documenté)
- **gpiozero** : Alternative plus haut niveau, peut être utilisé pour simplifier certains cas

### E-Paper

- **waveshare-epd** : Si compatible avec GDEY037T03-FT21
- **Driver personnalisé** : Probablement nécessaire (basé sur datasheet UC8253)

### NFC

- **nfcpy** : Alternative générique, mais Adafruit plus simple
- **libnfc** : Via bindings Python, plus complexe

---

## Outils de développement

### Linting et formatage

| Outil | Usage | Installation |
|-------|-------|--------------|
| `pylint` | Linting Python | `pip install pylint` |
| `flake8` | Linting Python (alternative) | `pip install flake8` |
| `black` | Formatage Python | `pip install black` |
| `shellcheck` | Linting Bash | `apt-get install shellcheck` |

### Tests (optionnel)

| Outil | Usage | Installation |
|-------|-------|--------------|
| `pytest` | Tests unitaires | `pip install pytest` |
| `pytest-cov` | Couverture de code | `pip install pytest-cov` |

---

## Structure des dépendances

### requirements.txt (à créer)

```txt
# Communication
requests>=2.31.0

# Matériel - Bus
smbus2>=0.4.3
spidev>=3.6
RPi.GPIO>=0.7.1

# Matériel - Périphériques
adafruit-circuitpython-bme280>=2.5.11
adafruit-circuitpython-pn532>=1.4.0

# Graphisme
Pillow>=10.0.0

# Utilitaires
python-dateutil>=2.8.2
```

### Installation dans Dockerfile

```dockerfile
RUN pip3 install --no-cache-dir -r requirements.txt
```

---

## Compatibilité

### Python

- **Minimum** : Python 3.9
- **Recommandé** : Python 3.11+
- **Testé sur** : Python 3.9, 3.10, 3.11 (selon images HA)

### Architecture

- **Principal** : aarch64 (Raspberry Pi 4/5)
- **Support futur** : amd64 (pour développement/test)

### Home Assistant

- **Version minimum** : Home Assistant OS 12+
- **API Supervisor** : Compatible avec versions récentes

---

## Performance et contraintes

### E-Paper

- **Rafraîchissement lent** : Optimiser les mises à jour
- **1-bit uniquement** : Conversion d'images nécessaire
- **Consommation** : Très faible, idéal pour embarqué

### I2C

- **Bus partagé** : Tous les périphériques sur le même bus
- **Vitesse** : 100kHz standard, 400kHz fast mode
- **Gestion** : Polling ou interrupt selon périphérique

### GPIO/PWM

- **Hardware PWM** : Préféré pour précision (GPIO 18)
- **Software PWM** : Alternative si hardware non disponible
- **Timing critique** : WS2812B nécessite timing précis

---

*Dernière mise à jour : 2026-01-17*
