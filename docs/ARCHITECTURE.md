# Architecture technique - HA Box

Ce document décrit l'architecture technique de l'add-on HA Box.

## Vue d'ensemble

```
┌─────────────────────────────────────────────────────────────────┐
│                     Home Assistant OS                            │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    Supervisor                              │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │              HA Box Add-on (Container)              │  │  │
│  │  │                                                     │  │  │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐            │  │  │
│  │  │  │ Display │  │ Sensors │  │ Control │            │  │  │
│  │  │  │ Manager │  │ Manager │  │ Manager │            │  │  │
│  │  │  └────┬────┘  └────┬────┘  └────┬────┘            │  │  │
│  │  │       │            │            │                  │  │  │
│  │  │  ┌────┴────────────┴────────────┴────┐            │  │  │
│  │  │  │         Hardware Abstraction       │            │  │  │
│  │  │  │              Layer (HAL)           │            │  │  │
│  │  │  └────────────────┬───────────────────┘            │  │  │
│  │  └───────────────────┼───────────────────────────────┘  │  │
│  └──────────────────────┼──────────────────────────────────┘  │
│                         │                                      │
│  ┌──────────────────────┼──────────────────────────────────┐  │
│  │                 Linux Kernel                             │  │
│  │    /dev/spidev0.0  /dev/i2c-1  /sys/class/gpio          │  │
│  └──────────────────────┼──────────────────────────────────┘  │
└─────────────────────────┼───────────────────────────────────────┘
                          │
    ┌─────────────────────┼─────────────────────┐
    │              Raspberry Pi                  │
    │  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ │
    │  │ Écran │ │  NFC  │ │ Temp  │ │  LED  │ │
    │  │  SPI  │ │  I2C  │ │  I2C  │ │  PWM  │ │
    │  └───────┘ └───────┘ └───────┘ └───────┘ │
    │  ┌───────┐ ┌───────┐                     │
    │  │Tactile│ │ Vent. │                     │
    │  │  I2C  │ │  PWM  │                     │
    │  └───────┘ └───────┘                     │
    └───────────────────────────────────────────┘
```

## Composants principaux

### 1. Display Manager

Responsable de l'affichage sur l'écran SPI.

**Responsabilités :**
- Initialisation de l'écran
- Rendu graphique (texte, images, icônes)
- Gestion des pages/écrans
- Rafraîchissement optimisé

**Technologies envisagées :**
- Python + Pillow pour le rendu
- Framebuffer Linux
- Ou bibliothèque dédiée (luma.lcd, etc.)

### 2. Sensors Manager

Gère la lecture des capteurs.

**Responsabilités :**
- Lecture périodique des capteurs I2C
- Mise en cache des valeurs
- Publication vers Home Assistant
- Gestion des erreurs de lecture

**Capteurs gérés :**
- Température (I2C)
- NFC (I2C)
- Tactile (I2C)

### 3. Control Manager

Gère les sorties et actuateurs.

**Responsabilités :**
- Contrôle du ventilateur PWM
- Contrôle de la bande LED
- Régulation automatique
- Réponse aux commandes HA

### 4. Hardware Abstraction Layer (HAL)

Couche d'abstraction pour l'accès matériel.

**Responsabilités :**
- Abstraction des bus (I2C, SPI, GPIO)
- Gestion des permissions
- Détection du matériel
- Fallback et gestion d'erreurs

## Communication avec Home Assistant

### API Supervisor

L'add-on communique avec Home Assistant via l'API Supervisor :

```
┌─────────────┐     HTTP/REST      ┌─────────────┐
│   HA Box    │ ◄────────────────► │  Supervisor │
│   Add-on    │                    │     API     │
└─────────────┘                    └──────┬──────┘
                                          │
                                   ┌──────┴──────┐
                                   │  HA Core    │
                                   │   (API)     │
                                   └─────────────┘
```

### Endpoints utilisés

| Endpoint | Usage |
|----------|-------|
| `/core/api/states` | Lecture des états des entités |
| `/core/api/services` | Appel de services |
| `/core/api/events` | Envoi d'événements (NFC) |
| `/addons/self/options` | Lecture de la configuration |

### Authentification

- Token Supervisor via variable d'environnement `SUPERVISOR_TOKEN`
- Accès automatique depuis le conteneur add-on

## Structure des fichiers

```
ha-box/
├── config.yaml           # Configuration de l'add-on
├── build.yaml            # Configuration de build
├── Dockerfile            # Image Docker
├── apparmor.txt          # Profil AppArmor
├── CHANGELOG.md          # Historique des versions
├── DOCS.md               # Documentation utilisateur
├── README.md             # Présentation
├── icon.png              # Icône 256x256
├── logo.png              # Logo 256x256
├── translations/
│   ├── en.yaml           # Traductions anglais
│   └── fr.yaml           # Traductions français
└── rootfs/
    ├── etc/
    │   └── services.d/
    │       └── ha-box/
    │           ├── run       # Script de démarrage
    │           └── finish    # Script de fin
    └── usr/
        └── bin/
            └── ha-box/
                ├── main.py           # Point d'entrée
                ├── config.py         # Gestion configuration
                ├── display/
                │   ├── __init__.py
                │   ├── manager.py    # Display Manager
                │   ├── screens/      # Écrans/pages
                │   └── drivers/      # Drivers d'écran
                ├── sensors/
                │   ├── __init__.py
                │   ├── manager.py    # Sensors Manager
                │   ├── temperature.py
                │   ├── nfc.py
                │   └── touch.py
                ├── control/
                │   ├── __init__.py
                │   ├── manager.py    # Control Manager
                │   ├── fan.py
                │   └── led.py
                ├── hal/
                │   ├── __init__.py
                │   ├── i2c.py
                │   ├── spi.py
                │   └── gpio.py
                └── ha/
                    ├── __init__.py
                    └── client.py     # Client API HA
```

## Configuration matérielle requise

### Raspberry Pi config.txt

L'utilisateur devra ajouter dans `/mnt/boot/config.txt` :

```ini
# Activer I2C
dtparam=i2c_arm=on
dtparam=i2c1=on

# Activer SPI
dtparam=spi=on

# PWM pour ventilateur (GPIO 18)
dtoverlay=pwm,pin=18,func=2

# Selon l'écran utilisé
# dtoverlay=spi0-1cs
```

### Adresses I2C prévues

| Périphérique | Adresse | Notes |
|--------------|---------|-------|
| Tactile | 0x38 | FT6236 typique |
| NFC | 0x24 | PN532 en mode I2C |
| Température | 0x76/0x77 | BME280 |

### Pins GPIO utilisées

| Pin | Fonction | Périphérique |
|-----|----------|--------------|
| GPIO 10 (SPI MOSI) | Data | Écran |
| GPIO 11 (SPI SCLK) | Clock | Écran |
| GPIO 8 (SPI CE0) | Chip Select | Écran |
| GPIO 25 | DC | Écran |
| GPIO 24 | Reset | Écran |
| GPIO 2 (SDA) | I2C Data | Tactile, NFC, Temp |
| GPIO 3 (SCL) | I2C Clock | Tactile, NFC, Temp |
| GPIO 18 | PWM | Ventilateur |
| GPIO 21 | Data | LED WS2812 |

## Séquence de démarrage

```
┌────────────────────────────────────────────────────────────────┐
│ 1. Supervisor démarre le conteneur                             │
├────────────────────────────────────────────────────────────────┤
│ 2. s6-overlay initialise les services                          │
├────────────────────────────────────────────────────────────────┤
│ 3. Script 'run' exécute main.py                                │
├────────────────────────────────────────────────────────────────┤
│ 4. HAL : Détection et initialisation du matériel               │
│    - Vérification I2C disponible                               │
│    - Vérification SPI disponible                               │
│    - Scan des périphériques                                    │
├────────────────────────────────────────────────────────────────┤
│ 5. Display Manager : Initialisation écran                      │
│    - Affichage écran de boot                                   │
├────────────────────────────────────────────────────────────────┤
│ 6. Sensors Manager : Démarrage lectures                        │
│    - Premier relevé température                                │
│    - Initialisation NFC                                        │
│    - Calibration tactile                                       │
├────────────────────────────────────────────────────────────────┤
│ 7. Control Manager : Initialisation sorties                    │
│    - Configuration PWM ventilateur                             │
│    - Initialisation LED                                        │
├────────────────────────────────────────────────────────────────┤
│ 8. Connexion à Home Assistant API                              │
│    - Attente si HA pas encore prêt                             │
│    - Récupération des entités configurées                      │
├────────────────────────────────────────────────────────────────┤
│ 9. Boucle principale                                           │
│    - Mise à jour affichage                                     │
│    - Lecture capteurs                                          │
│    - Traitement événements tactile/NFC                         │
│    - Régulation ventilateur                                    │
└────────────────────────────────────────────────────────────────┘
```

## Gestion des erreurs

### Stratégie de fallback

| Erreur | Comportement |
|--------|--------------|
| Écran non détecté | Log warning, mode headless |
| I2C indisponible | Log erreur, désactivation capteurs I2C |
| HA non accessible | Retry avec backoff, affichage mode dégradé |
| Capteur en erreur | Skip lecture, utiliser dernière valeur |

### Logging

Utilisation de `bashio::log.*` pour les scripts bash et module `logging` Python :

```python
import logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("ha-box")
```

Niveaux :
- `DEBUG` : Détails techniques
- `INFO` : Événements normaux
- `WARNING` : Problèmes non bloquants
- `ERROR` : Erreurs récupérables
- `CRITICAL` : Erreurs fatales

## Sécurité

### Permissions requises

```yaml
# config.yaml
devices:
  - /dev/i2c-1
  - /dev/spidev0.0
  - /dev/gpiomem
gpio: true
kernel_modules: true
```

### AppArmor

Profil AppArmor personnalisé pour limiter les accès :
- Accès en lecture/écriture aux périphériques déclarés
- Accès réseau limité à l'API Supervisor
- Pas d'accès au système de fichiers host

---

## Évolutions futures

- [ ] Support de plusieurs types d'écrans
- [ ] Plugin système pour drivers additionnels
- [ ] Mode simulation pour développement sans matériel
- [ ] Interface web de configuration avancée

---

*Dernière mise à jour : 2026-01-17*
