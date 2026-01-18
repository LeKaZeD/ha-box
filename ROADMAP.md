# Feuille de route - HA Box

Ce document décrit l'état actuel du projet et les prochaines étapes de développement.

## 📊 État actuel du projet

### ✅ Phase 1 : Conception et documentation (TERMINÉE)

| Tâche | Statut | Notes |
|-------|--------|-------|
| Documentation du projet | ✅ | `docs/PROJECT.md` |
| Cahier des fonctionnalités | ✅ | `docs/FEATURES.md` (8 features définies) |
| Architecture technique | ✅ | `docs/ARCHITECTURE.md` |
| Spécifications matérielles | ✅ | `docs/HARDWARE.md` |
| Guide de contribution | ✅ | `CONTRIBUTING.md` |
| Règles de développement | ✅ | `.cursorrules` |
| README principal | ✅ | `README.md` |

**Résultat** : Documentation complète, matériel identifié, architecture définie.

---

## 🛠️ Stack technique

### Langages et technologies

| Composant | Technologie | Version | Usage |
|-----------|-------------|---------|-------|
| **Conteneur** | Docker | Latest | Basé sur images Home Assistant |
| **Init system** | s6-overlay | v3 | Gestion des services |
| **Scripts système** | Bash | 5.x | Scripts de démarrage/arrêt |
| **Application principale** | Python | 3.9+ | Application métier |
| **Configuration** | YAML | - | Config add-on, build |
| **Build** | Docker Buildx | - | Build multi-arch (aarch64) |

### Bibliothèques Python prévues

| Bibliothèque | Usage | Installation |
|--------------|-------|--------------|
| `bashio` | Accès API Supervisor | Inclus dans images HA |
| `requests` | Communication HTTP avec HA | `pip install requests` |
| `Pillow` | Rendu graphique E-Paper | `pip install Pillow` |
| `adafruit-circuitpython-bme280` | Capteur BME280 | `pip install adafruit-circuitpython-bme280` |
| `adafruit-circuitpython-pn532` | Module NFC PN532 | `pip install adafruit-circuitpython-pn532` |
| `RPi.GPIO` ou `gpiozero` | Accès GPIO | `pip install RPi.GPIO` |
| `spidev` | Accès SPI | `pip install spidev` |
| `smbus2` | Accès I2C | `pip install smbus2` |

### Outils de développement

- **Linting** : `pylint`, `flake8`, `shellcheck`
- **Formatage** : `black` (Python), formatage automatique Bash
- **Tests** : `pytest` (si tests unitaires ajoutés)
- **Versioning** : Git avec Gitflow

---

## 🎯 Phases de développement

### Phase 2 : Infrastructure de base (✅ TERMINÉE)

**Objectif** : Créer la structure de base de l'add-on et l'infrastructure minimale.

| Tâche | Priorité | Statut | Notes |
|-------|----------|--------|-------|
| Créer structure `ha-box/` | 🔴 Critique | ✅ Terminé | Structure de base créée |
| Configurer `config.yaml` | 🔴 Critique | ✅ Terminé | Permissions, devices, options configurés |
| Configurer `build.yaml` | 🔴 Critique | ✅ Terminé | Build multi-arch configuré |
| Créer `Dockerfile` | 🔴 Critique | ✅ Terminé | Image avec dépendances Python |
| Scripts s6 (`run`, `finish`) | 🔴 Critique | ✅ Terminé | Démarrage/arrêt implémentés |
| Structure Python de base | 🔴 Critique | ✅ Terminé | `main.py`, modules de base créés |
| HAL (Hardware Abstraction Layer) | 🟠 Haute | ✅ Terminé | `hal/i2c.py`, `hal/spi.py`, `hal/gpio.py` |
| Client API Home Assistant | 🟠 Haute | ✅ Terminé | `ha/client.py` implémenté |
| Gestion de configuration | 🟠 Haute | ✅ Terminé | `config.py` avec chargement options |
| Logging et gestion d'erreurs | 🟠 Haute | ✅ Terminé | Infrastructure de logging créée |

**Durée réelle** : Terminée

**Résultat** : Infrastructure complète créée, prête pour Phase 3 (support matériel)

---

### Phase 3 : Support matériel de base (EN COURS)

**Objectif** : Implémenter le support des périphériques matériels essentiels.

#### 3.1 Capteur BME280 (Priorité haute)

| Tâche | Statut | Notes |
|-------|--------|-------|
| Driver BME280 | ⏳ À faire | Lecture température, humidité, pression |
| Détection automatique adresse I2C | ⏳ À faire | 0x76 ou 0x77 |
| Exposition comme sensors HA | ⏳ À faire | 3 entités HA |
| Gestion erreurs | ⏳ À faire | Timeout, déconnexion |

#### 3.2 Écran E-Paper (Priorité haute)

| Tâche | Statut | Notes |
|-------|--------|-------|
| Driver UC8253 | ⏳ À faire | Communication SPI, initialisation |
| Rendu graphique | ⏳ À faire | Conversion image → 1-bit |
| Gestion rafraîchissement | ⏳ À faire | Complet/partiel, optimisation |
| Front-light PWM | ⏳ À faire | Contrôle MOSFET via PWM |
| Écran de boot | ⏳ À faire | Affichage au démarrage |

#### 3.3 Tactile FT6336U (Priorité moyenne)

| Tâche | Statut | Notes |
|-------|--------|-------|
| Driver FT6336U | ⏳ À faire | Lecture touches I2C |
| Calibration | ⏳ À faire | Mapping 240×416 |
| Gestion gestes | ⏳ À faire | Tap, long press, swipe |

**Durée estimée** : 3-4 semaines

---

### Phase 4 : Fonctionnalités avancées

**Objectif** : Ajouter les fonctionnalités complémentaires.

#### 4.1 NFC PN532

| Tâche | Statut | Notes |
|-------|--------|-------|
| Driver PN532 | ⏳ À faire | Communication I2C |
| Détection tags | ⏳ À faire | Polling continu |
| Intégration HA | ⏳ À faire | Événements HA |
| Support protocoles | ⏳ À faire | MIFARE, NTAG |

#### 4.2 Ventilateur PWM

| Tâche | Statut | Notes |
|-------|--------|-------|
| Contrôle PWM | ⏳ À faire | GPIO 18, hardware PWM |
| Régulation automatique | ⏳ À faire | Basée sur température |
| Exposition comme fan HA | ⏳ À faire | Entité fan |

#### 4.3 Bande LED (Optionnel)

| Tâche | Statut | Notes |
|-------|--------|-------|
| Driver WS2812B | ⏳ À faire | Timing critique |
| Effets | ⏳ À faire | Fixe, fade, rainbow |
| Exposition comme light HA | ⏳ À faire | Entité light |

**Durée estimée** : 2-3 semaines

---

### Phase 5 : Interface utilisateur et intégration

**Objectif** : Créer l'interface utilisateur sur l'écran E-Paper.

| Tâche | Statut | Notes |
|-------|--------|-------|
| Système d'écrans | ⏳ À faire | Pages multiples |
| Écran principal | ⏳ À faire | Dashboard HA |
| Navigation tactile | ⏳ À faire | Swipe entre écrans |
| Affichage entités HA | ⏳ À faire | Sélection configurable |
| Widgets | ⏳ À faire | Température, horloge, etc. |
| Configuration UI | ⏳ À faire | Options dans add-on |

**Durée estimée** : 2-3 semaines

---

### Phase 6 : Tests et optimisation

**Objectif** : Tester sur matériel réel et optimiser.

| Tâche | Statut | Notes |
|-------|--------|-------|
| Tests matériel réel | ⏳ À faire | Raspberry Pi 4/5 |
| Tests d'intégration | ⏳ À faire | Tous les périphériques |
| Optimisation performance | ⏳ À faire | Rafraîchissement E-Paper |
| Gestion erreurs robuste | ⏳ À faire | Fallbacks, retry |
| Documentation utilisateur | ⏳ À faire | Guide d'installation |
| Tests de charge | ⏳ À faire | Stabilité long terme |

**Durée estimée** : 1-2 semaines

---

### Phase 7 : Release

**Objectif** : Préparer la première release.

| Tâche | Statut | Notes |
|-------|--------|-------|
| Version 0.1.0 alpha | ⏳ À faire | Version test |
| Version 1.0.0 | ⏳ À faire | Version stable |
| Documentation complète | ⏳ À faire | README, guides |
| CI/CD | ⏳ À faire | Build automatique |
| Changelog | ⏳ À faire | Historique versions |

**Durée estimée** : 1 semaine

---

## 📅 Timeline estimée

| Phase | Durée | Dépendances |
|-------|-------|-------------|
| Phase 1 : Documentation | ✅ Terminée | - |
| Phase 2 : Infrastructure | 1-2 semaines | Phase 1 |
| Phase 3 : Matériel de base | 3-4 semaines | Phase 2 |
| Phase 4 : Fonctionnalités avancées | 2-3 semaines | Phase 3 |
| Phase 5 : Interface utilisateur | 2-3 semaines | Phase 3 |
| Phase 6 : Tests | 1-2 semaines | Phase 4, 5 |
| Phase 7 : Release | 1 semaine | Phase 6 |

**Total estimé** : 10-15 semaines (2.5-4 mois)

---

## 🎯 Prochaines actions immédiates

1. **Renommer `example/` en `ha-box/`** et adapter la structure
2. **Configurer `config.yaml`** avec les permissions matérielles
3. **Créer le `Dockerfile`** avec les dépendances Python
4. **Implémenter la HAL** (Hardware Abstraction Layer) de base
5. **Créer `main.py`** avec la boucle principale

---

## 📝 Notes importantes

- **Matériel requis** : Tester sur Raspberry Pi réel dès que possible
- **E-Paper** : Le rafraîchissement lent nécessite une interface adaptée
- **Priorités** : BME280 et Écran E-Paper en premier (fonctionnalités critiques)
- **Tests** : Tester chaque composant individuellement avant intégration

---

*Dernière mise à jour : 2026-01-17*
