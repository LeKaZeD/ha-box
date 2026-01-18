# Cahier des fonctionnalités - HA Box

Ce document liste toutes les fonctionnalités prévues, leur statut et leurs spécifications.

## Légende des statuts

| Statut | Description |
|--------|-------------|
| 📋 À définir | Besoin de spécifications |
| 🔜 Planifié | Spécifié, en attente de développement |
| 🚧 En cours | Développement actif |
| ✅ Terminé | Implémenté et testé |
| ❌ Abandonné | Non retenu |

## Légende des priorités

| Priorité | Description |
|----------|-------------|
| 🔴 Critique | Bloquant pour le projet |
| 🟠 Haute | Fonctionnalité principale |
| 🟡 Moyenne | Amélioration importante |
| 🟢 Basse | Nice to have |

---

## F001 - Écran E-Paper SPI

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟠 Haute |
| **Statut** | 🔜 Planifié |
| **Interface** | SPI 4-wire ou 3-wire (`/dev/spidev0.0`) |
| **Matériel** | GDEY037T03-FT21 (GooDisplay) |

### Description

Affichage graphique sur un écran E-Paper 3.7" avec front-light intégré. L'écran est bi-stable (conserve l'image sans alimentation) et permet d'afficher :
- Informations de Home Assistant (états, capteurs)
- Heure et date
- Messages personnalisés
- Icônes et graphiques simples (niveau de gris)

**Avantages E-Paper :**
- Consommation ultra-faible (34µA en veille, 1.1µA en deep sleep)
- Lisibilité parfaite en lumière ambiante
- Pas de rafraîchissement constant nécessaire
- Front-light intégré pour utilisation dans l'obscurité

### Spécifications techniques

| Paramètre | Valeur |
|-----------|--------|
| **Modèle** | GDEY037T03-FT21 |
| **Taille** | 3.7" |
| **Résolution** | 240×416 pixels |
| **DPI** | 130 |
| **Contrôleur** | UC8253 |
| **Interface** | SPI 4-wire ou 3-wire |
| **Front-light** | 9 LEDs, 2.8V (typique) |
| **Température** | -25°C à 70°C |
| **Pixels** | 1-bit (noir/blanc) |

**Bibliothèques envisagées :**
- `waveshare-epd` (si compatible)
- `epdlib` ou bibliothèque générique E-Paper
- Driver personnalisé basé sur la datasheet

**Rafraîchissement :**
- Rafraîchissement complet : ~2-3 secondes
- Rafraîchissement partiel : ~1 seconde (si supporté)
- Stratégie : Rafraîchir uniquement quand nécessaire (changement de données)

### Critères d'acceptation

- [ ] L'écran s'initialise au démarrage de l'add-on
- [ ] Affichage de texte lisible (niveau de gris)
- [ ] Affichage d'au moins 3 entités Home Assistant
- [ ] Mise à jour automatique des valeurs (rafraîchissement optimisé)
- [ ] Contrôle du front-light (on/off, intensité)
- [ ] Gestion du deep sleep pour économie d'énergie

### Dépendances

- Configuration SPI activée sur le Pi
- Accès au périphérique `/dev/spidev0.0`
- Pins de contrôle (DC, Reset, BUSY) via GPIO

### Notes techniques

- Le contrôleur UC8253 nécessite une séquence d'initialisation spécifique
- Signal BUSY à surveiller pour synchronisation
- Waveform stockée dans OTP ou chargée par MCU
- Support du mode portrait et paysage

---

## F002 - Interface tactile I2C

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟡 Moyenne |
| **Statut** | 🔜 Planifié |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Matériel** | FT6336U (intégré dans GDEY037T03-FT21) |

### Description

Gestion des entrées tactiles via le contrôleur FT6336U intégré à l'écran E-Paper. Permet une interaction simple avec l'interface :
- Navigation entre écrans
- Sélection d'éléments
- Actions rapides (toggle, slider)
- **Note** : Le feedback visuel sera limité par la vitesse de rafraîchissement de l'E-Paper

### Spécifications techniques

| Paramètre | Valeur |
|-----------|--------|
| **Contrôleur** | FT6336U |
| **Interface** | I2C |
| **Tension** | 3.0V |
| **Résolution écran** | 240×416 pixels (mapping tactile) |

**Bibliothèques envisagées :**
- `ft6336` (driver Python)
- Driver basé sur datasheet FT6336U

**Gestion des gestes :**
- Tap simple
- Long press
- Swipe (limité par rafraîchissement E-Paper)

### Critères d'acceptation

- [ ] Détection des touches
- [ ] Précision acceptable (±5px)
- [ ] Réponse < 100ms (lecture I2C)
- [ ] Au moins 3 gestes supportés (tap, long press, swipe)
- [ ] Mapping correct avec la résolution 240×416

### Dépendances

- F001 (Écran E-Paper) - le tactile est intégré
- Configuration I2C activée
- Adresse I2C du FT6336U (à vérifier dans datasheet)

### Notes techniques

- Le FT6336U est intégré au module, pas besoin de composant séparé
- Adresse I2C typique : 0x38 (à confirmer)
- Support multi-touch (2 points simultanés)

---

## F003 - Capteur NFC PN532

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟡 Moyenne |
| **Statut** | 🔜 Planifié |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Matériel** | PN532 (NXP) |

### Description

Lecture de tags NFC via le module PN532 pour déclencher des actions dans Home Assistant :
- Identification de tags
- Déclenchement d'automatisations
- Authentification simple
- Support de multiples protocoles NFC

### Spécifications techniques

| Paramètre | Valeur |
|-----------|--------|
| **Modèle** | PN532 |
| **Interface** | I2C |
| **Adresse I2C** | 0x24 (typique) |
| **Protocoles** | MIFARE Classic, NTAG21x, ISO14443 Type A/B |
| **Portée** | ~5cm |
| **Tension** | 3.3V ou 5V (selon module) |

**Bibliothèques envisagées :**
- `adafruit-circuitpython-pn532` (Adafruit)
- `nfcpy` (driver Python générique)
- `libnfc` (via bindings Python)

**Mode de lecture :**
- Polling continu (détection de tags)
- Mode interrupt (si supporté par le module)
- Fréquence de scan : 1-2 Hz (configurable)

### Critères d'acceptation

- [ ] Lecture de tags MIFARE Classic
- [ ] Lecture de tags NTAG21x
- [ ] Lecture de tags ISO14443 Type A
- [ ] Envoi d'événement à Home Assistant avec UID du tag
- [ ] Temps de lecture < 500ms
- [ ] Détection automatique de la présence de tags

### Dépendances

- Configuration I2C activée
- API Home Assistant accessible
- Module PN532 configuré en mode I2C (jumpers/sélecteurs)

### Notes techniques

- Le PN532 peut fonctionner en I2C, SPI ou UART selon la configuration
- Vérifier les jumpers/sélecteurs du module pour le mode I2C
- Adresse I2C peut varier selon le module (0x24 ou 0x48)
- Consommation : ~15mA en mode actif

---

## F004 - Capteur BME280 (Température/Humidité/Pression)

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟠 Haute |
| **Statut** | 🔜 Planifié |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Matériel** | BME280 (Bosch) |

### Description

Mesure de la température, humidité et pression atmosphérique via le capteur BME280 pour :
- Affichage sur l'écran E-Paper
- Exposition comme entités Home Assistant (3 sensors)
- Régulation du ventilateur (F006) basée sur température
- Suivi des conditions ambiantes

### Spécifications techniques

| Paramètre | Valeur |
|-----------|--------|
| **Modèle** | BME280 |
| **Interface** | I2C (ou SPI, mais I2C choisi) |
| **Adresses I2C** | 0x76 ou 0x77 (selon configuration) |
| **Température** | -40°C à +85°C |
| **Précision température** | ±1°C |
| **Humidité** | 0-100% RH |
| **Précision humidité** | ±3% RH |
| **Pression** | 300-1100 hPa |
| **Précision pression** | ±1 hPa |

**Bibliothèques envisagées :**
- `adafruit-circuitpython-bme280` (Adafruit)
- `bme280` (driver Python standard)
- `RPi.bme280` (spécifique Raspberry Pi)

**Fréquence de mesure :**
- Lecture toutes les 30 secondes (configurable)
- Mise en cache pour éviter surcharge I2C
- Mode sleep entre les lectures pour économie d'énergie

### Critères d'acceptation

- [ ] Lecture de température avec précision ±1°C
- [ ] Lecture d'humidité avec précision ±3% RH
- [ ] Lecture de pression avec précision ±1 hPa
- [ ] Exposition comme 3 sensors dans HA (`sensor.ha_box_temperature`, `sensor.ha_box_humidity`, `sensor.ha_box_pressure`)
- [ ] Mise à jour toutes les 30s minimum
- [ ] Affichage sur l'écran local (valeurs formatées)
- [ ] Détection automatique de l'adresse I2C (0x76 ou 0x77)

### Dépendances

- Configuration I2C activée
- F001 (optionnel, pour affichage)
- Pull-ups I2C (généralement présents sur modules BME280)

### Notes techniques

- Le BME280 nécessite une calibration initiale (compensation)
- Support du mode forced (mesure à la demande) ou normal (mesure continue)
- Filtre configurable pour lisser les valeurs

---

## F005 - Bande LED

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟢 Basse |
| **Statut** | 📋 À définir |
| **Interface** | GPIO/PWM ou SPI |
| **Matériel** | WS2812B, SK6812, APA102, etc. |

### Description

Contrôle d'une bande LED pour :
- Notifications visuelles
- Ambiance lumineuse
- Indicateur de statut

### Spécifications techniques

- [ ] Type de LED (WS2812B recommandé)
- [ ] Nombre de LEDs maximum
- [ ] Méthode de contrôle (PWM, SPI, DMA)
- [ ] Effets disponibles

### Critères d'acceptation

- [ ] Contrôle de couleur RGB
- [ ] Au moins 3 effets (fixe, fade, rainbow)
- [ ] Intégration comme light dans HA
- [ ] Réactivité < 50ms

### Dépendances

- Accès GPIO ou SPI
- Alimentation suffisante

---

## F006 - Ventilateur PWM

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟡 Moyenne |
| **Statut** | 📋 À définir |
| **Interface** | GPIO PWM |
| **Matériel** | Ventilateur 5V PWM 4 pins |

### Description

Régulation de la vitesse du ventilateur basée sur :
- Température du CPU
- Température ambiante (F004)
- Contrôle manuel

### Spécifications techniques

- [ ] Pin GPIO PWM à utiliser
- [ ] Fréquence PWM
- [ ] Courbe de régulation
- [ ] Seuils de température

### Critères d'acceptation

- [ ] Contrôle de vitesse 0-100%
- [ ] Régulation automatique basée sur température
- [ ] Mode manuel disponible
- [ ] Exposition comme fan dans HA

### Dépendances

- Accès GPIO PWM
- F004 (pour régulation automatique)

---

## F007 - Configuration et UI

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟠 Haute |
| **Statut** | 📋 À définir |
| **Interface** | Home Assistant |

### Description

Interface de configuration de l'add-on :
- Options dans le panneau de l'add-on
- Sélection des entités à afficher
- Configuration des seuils et paramètres

### Spécifications techniques

- [ ] Schema de configuration
- [ ] Validation des entrées
- [ ] Rechargement à chaud
- [ ] Traductions (FR, EN)

### Critères d'acceptation

- [ ] Configuration fonctionnelle via UI
- [ ] Validation des erreurs
- [ ] Documentation des options
- [ ] Au moins 2 langues

### Dépendances

- Structure de base de l'add-on

---

## F008 - Démarrage précoce

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟢 Basse |
| **Statut** | 📋 À définir |
| **Interface** | Supervisor |

### Description

Permettre à l'add-on de démarrer le plus tôt possible pour :
- Afficher un écran de boot
- Initialiser les périphériques rapidement
- Afficher le statut de démarrage de HA

### Spécifications techniques

- [ ] Valeur de `startup` dans config.yaml
- [ ] Gestion de l'indisponibilité de HA
- [ ] Écran de fallback

### Critères d'acceptation

- [ ] L'écran affiche quelque chose dès le boot de l'add-on
- [ ] Pas de crash si HA n'est pas encore prêt
- [ ] Transition fluide vers l'écran principal

### Dépendances

- F001 (Écran SPI)
- Compréhension du cycle de boot Supervisor

---

## F008 - Front-light de l'écran

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟡 Moyenne |
| **Statut** | 🔜 Planifié |
| **Interface** | GPIO PWM |
| **Matériel** | 9 LEDs intégrées (2.8V) contrôlées par MOSFET |

### Description

Contrôle du front-light intégré à l'écran E-Paper pour permettre la lecture dans l'obscurité. Le front-light est contrôlé via un MOSFET qui bloque le courant par défaut, permettant un contrôle PWM pour régler l'intensité :
- Activation/désactivation
- Réglage de l'intensité via PWM (0-100%)
- Mode automatique basé sur luminosité ambiante (si capteur disponible)
- Économie d'énergie (désactivation automatique)

### Spécifications techniques

| Paramètre | Valeur |
|-----------|--------|
| **Contrôle** | MOSFET (PWM) |
| **Tension LEDs** | 2.8V typique |
| **Nombre de LEDs** | 9 |
| **Consommation max** | ~20-30mA |
| **Interface** | GPIO PWM (Hardware ou Software PWM) |

**Contrôle PWM :**
- Fréquence PWM : 1-10 kHz (à optimiser pour éviter scintillement)
- Résolution : 8-12 bits (256-4096 niveaux)
- Duty cycle : 0-100% (0% = éteint, 100% = max)

**Bibliothèques envisagées :**
- `RPi.GPIO` avec software PWM
- Hardware PWM du Raspberry Pi (si pin disponible)
- `pigpio` pour PWM plus précis

### Critères d'acceptation

- [ ] Contrôle on/off fonctionnel
- [ ] Intensité réglable via PWM (0-100%)
- [ ] Pas de scintillement visible à l'œil
- [ ] Intégration dans la configuration de l'add-on
- [ ] Mode automatique (on/off selon heure ou luminosité)

### Dépendances

- F001 (Écran E-Paper)
- Pin GPIO disponible pour PWM
- Configuration PWM activée

### Notes techniques

- Le MOSFET bloque le courant par défaut (état bas = éteint)
- PWM permet un contrôle fluide de l'intensité
- Éviter les fréquences trop basses (< 100Hz) pour éviter le scintillement
- Hardware PWM recommandé si disponible (plus précis, moins de charge CPU)

---

## Spécificités E-Paper

### Contraintes et opportunités

L'utilisation d'un écran E-Paper apporte des contraintes mais aussi des avantages uniques :

**Contraintes :**
- ⚠️ **Rafraîchissement lent** : 2-3 secondes pour un rafraîchissement complet
- ⚠️ **Affichage 1-bit** : Noir et blanc uniquement, pas de couleurs
- ⚠️ **Ghosting** : Traces d'images précédentes possibles (nécessite rafraîchissement périodique)
- ⚠️ **Température** : Performance dégradée en dessous de 0°C

**Avantages :**
- ✅ **Consommation ultra-faible** : 34µA en veille, 1.1µA en deep sleep
- ✅ **Lisibilité parfaite** : Excellent contraste en lumière ambiante
- ✅ **Bi-stable** : L'image reste affichée sans alimentation
- ✅ **Pas d'éblouissement** : Confortable pour lecture prolongée
- ✅ **Idéal pour affichage statique** : Parfait pour dashboard Home Assistant

### Stratégies d'optimisation

1. **Rafraîchissement intelligent** :
   - Rafraîchir uniquement les zones modifiées (si supporté)
   - Rafraîchissement complet périodique pour éviter le ghosting
   - Détection des changements significatifs avant rafraîchissement

2. **Interface utilisateur adaptée** :
   - Design minimaliste, optimisé pour noir/blanc
   - Utilisation de contrastes forts
   - Éviter les animations rapides
   - Feedback tactile/haptique pour compenser la latence visuelle

3. **Gestion de l'énergie** :
   - Mode deep sleep quand l'écran n'est pas utilisé
   - Désactivation du front-light quand non nécessaire
   - Rafraîchissement uniquement lors de changements importants

---

## Backlog / Idées futures

Ces fonctionnalités ne sont pas planifiées mais pourraient être ajoutées :

| ID | Fonctionnalité | Description |
|----|----------------|-------------|
| F009 | Boutons physiques | Support de boutons GPIO en plus du tactile |
| F010 | Audio | Sortie audio pour notifications sonores |
| F011 | Thèmes | Personnalisation de l'interface (niveaux de gris via dithering) |
| F012 | Widgets | Widgets personnalisables sur l'écran |
| F013 | Multi-écrans | Support de plusieurs écrans |
| F014 | Rafraîchissement partiel | Optimisation avec rafraîchissement partiel de l'E-Paper |
| F015 | Mode économie d'énergie | Détection d'inactivité et deep sleep automatique |

---

## Historique des modifications

| Date | Modification |
|------|--------------|
| 2026-01-17 | Création initiale du cahier de features |

---

*Ce document est vivant et sera mis à jour au fur et à mesure de l'avancement du projet.*
