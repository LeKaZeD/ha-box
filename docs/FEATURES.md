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

## F001 - Écran SPI

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟠 Haute |
| **Statut** | 📋 À définir |
| **Interface** | SPI (`/dev/spidev0.0`) |
| **Matériel** | À définir (ILI9341, ST7789, etc.) |

### Description

Affichage graphique sur un écran SPI connecté au Raspberry Pi. L'écran doit pouvoir afficher :
- Informations de Home Assistant (états, capteurs)
- Heure et date
- Messages personnalisés
- Icônes et graphiques simples

### Spécifications techniques

- [ ] Choix du contrôleur d'écran (ILI9341, ST7789, SSD1306...)
- [ ] Résolution cible
- [ ] Bibliothèque graphique (framebuffer, PIL, lvgl...)
- [ ] Rafraîchissement (fréquence, partiel/complet)

### Critères d'acceptation

- [ ] L'écran s'initialise au démarrage de l'add-on
- [ ] Affichage de texte lisible
- [ ] Affichage d'au moins 3 entités Home Assistant
- [ ] Mise à jour automatique des valeurs

### Dépendances

- Configuration SPI activée sur le Pi
- Accès au périphérique `/dev/spidev0.0`

---

## F002 - Interface tactile I2C

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟡 Moyenne |
| **Statut** | 📋 À définir |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Matériel** | À définir (FT6236, GT911, etc.) |

### Description

Gestion des entrées tactiles pour permettre une interaction simple avec l'interface :
- Navigation entre écrans
- Sélection d'éléments
- Actions rapides (toggle, slider)

### Spécifications techniques

- [ ] Choix du contrôleur tactile
- [ ] Calibration tactile
- [ ] Gestion des gestes (tap, swipe, long press)
- [ ] Mapping avec l'affichage

### Critères d'acceptation

- [ ] Détection des touches
- [ ] Précision acceptable (±5px)
- [ ] Réponse < 100ms
- [ ] Au moins 3 gestes supportés

### Dépendances

- F001 (Écran SPI) pour le feedback visuel
- Configuration I2C activée

---

## F003 - Capteur NFC I2C

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟡 Moyenne |
| **Statut** | 📋 À définir |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Matériel** | PN532, MFRC522, ou similaire |

### Description

Lecture de tags NFC pour déclencher des actions dans Home Assistant :
- Identification de tags
- Déclenchement d'automatisations
- Authentification simple

### Spécifications techniques

- [ ] Choix du module NFC
- [ ] Protocoles supportés (MIFARE, NTAG, etc.)
- [ ] Mode de lecture (polling vs interrupt)
- [ ] Intégration avec HA (events, tags)

### Critères d'acceptation

- [ ] Lecture de tags MIFARE Classic
- [ ] Lecture de tags NTAG21x
- [ ] Envoi d'événement à Home Assistant
- [ ] Temps de lecture < 500ms

### Dépendances

- Configuration I2C activée
- API Home Assistant accessible

---

## F004 - Capteur de température I2C

| Attribut | Valeur |
|----------|--------|
| **Priorité** | 🟠 Haute |
| **Statut** | 📋 À définir |
| **Interface** | I2C (`/dev/i2c-1`) |
| **Matériel** | BME280, BMP280, SHT31, DS18B20, etc. |

### Description

Mesure de la température (et optionnellement humidité/pression) pour :
- Affichage sur l'écran
- Exposition comme entité Home Assistant
- Régulation du ventilateur (F006)

### Spécifications techniques

- [ ] Choix du capteur
- [ ] Précision requise
- [ ] Fréquence de mesure
- [ ] Calibration/offset

### Critères d'acceptation

- [ ] Lecture de température avec précision ±0.5°C
- [ ] Exposition comme sensor dans HA
- [ ] Mise à jour toutes les 30s minimum
- [ ] Affichage sur l'écran local

### Dépendances

- Configuration I2C activée
- F001 (optionnel, pour affichage)

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

## Backlog / Idées futures

Ces fonctionnalités ne sont pas planifiées mais pourraient être ajoutées :

| ID | Fonctionnalité | Description |
|----|----------------|-------------|
| F009 | Boutons physiques | Support de boutons GPIO en plus du tactile |
| F010 | Audio | Sortie audio pour notifications sonores |
| F011 | Thèmes | Personnalisation de l'interface (couleurs, polices) |
| F012 | Widgets | Widgets personnalisables sur l'écran |
| F013 | Multi-écrans | Support de plusieurs écrans |

---

## Historique des modifications

| Date | Modification |
|------|--------------|
| 2026-01-17 | Création initiale du cahier de features |

---

*Ce document est vivant et sera mis à jour au fur et à mesure de l'avancement du projet.*
