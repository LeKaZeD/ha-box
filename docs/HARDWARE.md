# Matériel - HA Box

Ce document liste tous les composants matériels utilisés dans le projet HA Box avec leurs spécifications détaillées.

## Composants principaux

### 1. Écran E-Paper GDEY037T03-FT21

**Fabricant** : GooDisplay (Dalian Good Display Co., Ltd.)  
**Modèle** : GDEY037T03-FT21  
**Type** : E-Paper (Electrophoretic Display) avec front-light et tactile intégrés

#### Spécifications

| Paramètre | Valeur |
|-----------|--------|
| Taille | 3.7" |
| Résolution | 240×416 pixels |
| DPI | 130 |
| Zone active | 47.04×81.54 mm |
| Pitch pixel | 0.196×0.196 mm |
| Contrôleur | UC8253 |
| Interface | SPI 4-wire ou 3-wire |
| Front-light | 9 LEDs, 2.8V (typique) |
| Tactile | FT6336U (I2C, 3.0V) |
| Température opérationnelle | -25°C à 70°C |
| Consommation veille | 34µA |
| Consommation deep sleep | 1.1µA |

#### Caractéristiques E-Paper

- **Bi-stable** : L'image reste affichée sans alimentation
- **Rafraîchissement complet** : ~2-3 secondes
- **Rafraîchissement partiel** : ~1 seconde (si supporté)
- **Contraste élevé** : Excellent en lumière ambiante
- **Consommation ultra-faible** : Idéal pour applications embarquées
- **1-bit** : Affichage noir/blanc uniquement

#### Pins de connexion

| Pin | Nom | Fonction | Notes |
|-----|-----|----------|-------|
| SPI MOSI | Data | Données SPI | GPIO 10 |
| SPI SCLK | Clock | Horloge SPI | GPIO 11 |
| SPI CE0 | CS | Chip Select | GPIO 8 |
| DC | Data/Command | Sélection | GPIO (TBD) |
| Reset | Reset | Reset contrôleur | GPIO (TBD) |
| BUSY | Status | État contrôleur | GPIO (TBD, lecture) |
| TSCL | I2C Clock | Horloge I2C tactile | GPIO 3 |
| TSDA | I2C Data | Données I2C tactile | GPIO 2 |
| Front-light | PWM | Contrôle MOSFET front-light | GPIO (TBD, PWM) |
| VDD | Alimentation | 3.0V | Alimentation principale |
| GND | Masse | 0V | Masse commune |

#### Front-light

Le front-light est composé de 9 LEDs (2.8V typique) contrôlées par un MOSFET. Le MOSFET bloque le courant par défaut (état bas = éteint), permettant un contrôle PWM pour régler l'intensité lumineuse.

**Caractéristiques :**
- 9 LEDs intégrées
- Tension : 2.8V typique
- Contrôle : MOSFET avec PWM
- Consommation : ~20-30mA à intensité maximale
- Interface : GPIO avec PWM (Hardware ou Software PWM)

**Contrôle PWM :**
- Fréquence recommandée : 1-10 kHz (éviter scintillement)
- Résolution : 8-12 bits (256-4096 niveaux)
- Duty cycle : 0-100% (0% = éteint, 100% = max)

#### Documentation

- Datasheet : `GDEY037T03-FT21.pdf`
- Site web : [www.good-display.com](https://www.good-display.com)

---

### 2. Capteur BME280

**Fabricant** : Bosch  
**Modèle** : BME280  
**Type** : Capteur environnemental (Température, Humidité, Pression)

#### Spécifications

| Paramètre | Valeur |
|-----------|--------|
| Interface | I2C (ou SPI) |
| Adresses I2C | 0x76 ou 0x77 (selon configuration) |
| Température | -40°C à +85°C |
| Précision température | ±1°C |
| Humidité | 0-100% RH |
| Précision humidité | ±3% RH |
| Pression | 300-1100 hPa |
| Précision pression | ±1 hPa |
| Tension | 1.8V à 3.6V |
| Consommation | ~3.6µA (mode sleep) |

#### Connexion

| Pin | Fonction | Raspberry Pi |
|-----|----------|--------------|
| VCC | Alimentation 3.3V | 3.3V |
| GND | Masse | GND |
| SDA | I2C Data | GPIO 2 (SDA) |
| SCL | I2C Clock | GPIO 3 (SCL) |

#### Documentation

- Datasheet : Disponible sur le site Bosch
- Module Amazon : [BME280](https://www.amazon.fr/Gy-bme280-num%C3%A9rique-pr%C3%A9cision-barom%C3%A9trique-Temp%C3%A9rature/dp/B077PNKCQ6)

---

### 3. Module NFC PN532

**Fabricant** : NXP  
**Modèle** : PN532  
**Type** : Contrôleur NFC 13.56 MHz

#### Spécifications

| Paramètre | Valeur |
|-----------|--------|
| Interface | I2C (ou SPI/UART selon configuration) |
| Adresse I2C | 0x24 (typique, peut varier selon module) |
| Protocoles | MIFARE Classic, NTAG21x, ISO14443 Type A/B |
| Portée | ~5cm |
| Tension | 3.3V ou 5V (selon module) |
| Consommation | ~15mA (mode actif) |
| Fréquence | 13.56 MHz |

#### Caractéristiques

- Support de multiples protocoles NFC
- Mode polling pour détection automatique de tags
- Lecture/écriture de tags NFC
- Compatible avec la plupart des tags NFC standards

#### Connexion

| Pin | Fonction | Raspberry Pi |
|-----|----------|--------------|
| VCC | Alimentation | 3.3V ou 5V (selon module) |
| GND | Masse | GND |
| SDA | I2C Data | GPIO 2 (SDA) |
| SCL | I2C Clock | GPIO 3 (SCL) |

**Note** : Vérifier que le module est configuré en mode I2C (jumpers/sélecteurs sur le breakout board).

#### Documentation

- Datasheet : Disponible sur le site NXP
- Modules Amazon :
  - [Module NFC 1](https://www.amazon.fr/dp/B0FB95HMMC/)
  - [Module NFC 2](https://www.amazon.fr/dp/B0DJP3987K/)

---

### 4. Bande LED WS2812B (Optionnel)

**Type** : LED RGB adressable  
**Interface** : GPIO (protocole propriétaire)

#### Spécifications

| Paramètre | Valeur |
|-----------|--------|
| Interface | GPIO (1-wire) |
| Pin | GPIO 21 (proposé) |
| Tension | 5V |
| Consommation | ~60mA par LED (blanc max) |
| Protocole | WS2812B (timing critique) |

#### Notes

- Nécessite une bibliothèque dédiée (rpi_ws281x, neopixel)
- Timing précis requis (DMA recommandé)
- Alimentation externe recommandée pour >10 LEDs

---

### 5. Ventilateur PWM (Optionnel)

**Type** : Ventilateur 5V avec contrôle PWM

#### Spécifications

| Paramètre | Valeur |
|-----------|--------|
| Interface | GPIO PWM |
| Pin | GPIO 18 (Hardware PWM) |
| Tension | 5V |
| Contrôle | 0-100% vitesse |

#### Notes

- Utilise le PWM hardware du Raspberry Pi
- Régulation basée sur température (BME280 ou CPU)
- Peut être contrôlé manuellement via HA

---

## Configuration Raspberry Pi

### config.txt requis

```ini
# Activer I2C
dtparam=i2c_arm=on
dtparam=i2c1=on

# Activer SPI
dtparam=spi=on

# PWM pour ventilateur (GPIO 18)
dtoverlay=pwm,pin=18,func=2
```

### Périphériques Linux

| Périphérique | Usage |
|--------------|-------|
| `/dev/i2c-1` | Bus I2C principal (tactile, BME280, NFC) |
| `/dev/spidev0.0` | Bus SPI (écran E-Paper) |
| `/dev/gpiomem` | Accès GPIO (LED, front-light, contrôle) |

---

## Schéma de connexion (À compléter)

```
Raspberry Pi 4/5
├── SPI0
│   ├── MOSI (GPIO 10) ──> Écran E-Paper (Data)
│   ├── SCLK (GPIO 11) ──> Écran E-Paper (Clock)
│   └── CE0  (GPIO 8)  ──> Écran E-Paper (CS)
│
├── I2C1
│   ├── SDA (GPIO 2) ──> Tactile FT6336U, BME280, NFC
│   └── SCL (GPIO 3) ──> Tactile FT6336U, BME280, NFC
│
├── GPIO
│   ├── GPIO 18 ──> Ventilateur PWM
│   ├── GPIO 21 ──> LED WS2812B (optionnel)
│   └── GPIO (TBD) ──> Écran E-Paper (DC, Reset, BUSY, Front-light PWM)
│
└── Alimentation
    ├── 3.3V ──> BME280, Écran (VDD)
    ├── 5V ──> Ventilateur, LED (optionnel)
    └── GND ──> Tous les composants
```

---

## Consommation électrique estimée

| Composant | Consommation | Notes |
|-----------|--------------|-------|
| Écran E-Paper (veille) | 34µA | Deep sleep : 1.1µA |
| Écran E-Paper (rafraîchissement) | ~50mA | Pendant 2-3 secondes |
| Front-light | ~20-30mA | 9 LEDs à 2.8V (contrôlé par MOSFET PWM) |
| BME280 | ~3.6µA | Mode sleep |
| Tactile FT6336U | ~10µA | Mode sleep |
| NFC (PN532) | ~15mA | Mode actif |
| LED WS2812B | ~60mA/LED | Blanc max |
| Ventilateur | ~100-200mA | 5V PWM |

**Total estimé (veille)** : ~50µA (excellent pour applications embarquées)  
**Total estimé (actif)** : ~200-300mA (selon composants actifs)

---

## Notes importantes

1. **E-Paper** : Le rafraîchissement est lent (~2-3s), adapter l'interface utilisateur en conséquence
2. **I2C** : Tous les composants I2C partagent le même bus, vérifier les adresses
3. **Alimentation** : Le Raspberry Pi peut fournir 3.3V et 5V, mais vérifier les limites de courant
4. **Front-light** : Contrôlé via MOSFET avec PWM, tension 2.8V typique (9 LEDs intégrées)

---

*Dernière mise à jour : 2026-01-17*
