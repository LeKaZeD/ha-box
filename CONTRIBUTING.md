# Guide de contribution - HA Box

Bienvenue ! Ce document décrit les règles et processus pour contribuer au projet HA Box.

## 📋 Prérequis

### Connaissances requises

- **Home Assistant OS** : Compréhension des add-ons et de leur configuration
- **Linux embarqué** : Notions de base sur les bus I2C, SPI, GPIO
- **Docker** : Bases de la conteneurisation
- **Bash/Python** : Selon les composants à développer

### Matériel recommandé

- Raspberry Pi 4 ou 5 avec Home Assistant OS
- Écran SPI compatible (modèle à définir)
- Contrôleur tactile I2C
- Capteur NFC I2C (ex: PN532)
- Capteur de température I2C (ex: BME280, DS18B20)
- Bande LED adressable (WS2812B ou similaire)
- Ventilateur PWM 5V

### Environnement de développement

- Git configuré
- Accès SSH à votre instance Home Assistant
- IDE de votre choix (VSCode recommandé)
- Docker pour tests locaux (optionnel)

## 🌿 Workflow Git

### Branches

| Branche | Usage |
|---------|-------|
| `main` | Version stable, releases uniquement |
| `develop` | Développement actif, intégration |
| `feature/*` | Nouvelles fonctionnalités |
| `fix/*` | Corrections de bugs |
| `docs/*` | Documentation uniquement |

### Processus de contribution

1. **Fork** le dépôt (contributeurs externes)
2. **Créer une branche** depuis `develop` :
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/ma-fonctionnalite
   ```
3. **Développer** en suivant les conventions
4. **Tester** sur matériel réel si possible
5. **Commit** avec des messages clairs
6. **Push** et créer une **Pull Request** vers `develop`

### Messages de commit

Format : `type(scope): description`

Types :
- `feat` : Nouvelle fonctionnalité
- `fix` : Correction de bug
- `docs` : Documentation
- `refactor` : Refactoring
- `test` : Ajout/modification de tests
- `chore` : Maintenance

Exemples :
```
feat(display): ajout support écran ILI9341
fix(i2c): correction timeout lecture NFC
docs(readme): mise à jour installation
```

## 📝 Conventions de code

### Bash

- Utiliser `#!/usr/bin/with-contenv bashio` pour les scripts s6
- Indentation : 2 espaces
- Noms de variables : `UPPER_SNAKE_CASE` pour les constantes, `lower_snake_case` pour les variables
- Toujours quoter les variables : `"${variable}"`
- Utiliser `shellcheck` pour la validation

### Python

- Python 3.9+ minimum
- Style : PEP 8
- Utiliser `black` pour le formatage
- Utiliser `pylint` ou `flake8` pour la validation
- Type hints recommandés

### YAML

- Indentation : 2 espaces
- Pas de tabulations
- Commentaires explicatifs pour les options complexes

## 🔒 Sécurité et permissions

### Principe du moindre privilège

- Demander uniquement les permissions nécessaires
- Documenter chaque permission requise et pourquoi
- Utiliser AppArmor quand possible

### Périphériques matériels

Dans `config.yaml`, déclarer explicitement :
```yaml
devices:
  - /dev/i2c-1
  - /dev/spidev0.0
```

### Protection Mode

- Par défaut, garder le Protection Mode activé
- Si désactivation nécessaire, documenter les raisons

## 🧪 Tests

### Tests requis

1. **Tests unitaires** : Pour toute logique métier
2. **Tests d'intégration** : Communication avec les périphériques
3. **Tests manuels** : Sur matériel réel avant PR

### Checklist avant PR

- [ ] Code respecte les conventions
- [ ] Tests passent
- [ ] Documentation mise à jour
- [ ] Testé sur matériel (si applicable)
- [ ] Pas de credentials/secrets dans le code
- [ ] CHANGELOG.md mis à jour

## 📚 Documentation

### Fichiers à maintenir

| Fichier | Contenu |
|---------|---------|
| `README.md` | Vue d'ensemble du projet |
| `docs/PROJECT.md` | Vision et objectifs |
| `docs/FEATURES.md` | Cahier des fonctionnalités |
| `docs/ARCHITECTURE.md` | Architecture technique |
| `CONTRIBUTING.md` | Ce fichier |
| `CHANGELOG.md` | Historique des versions |

### Standards de documentation

- Markdown pour tous les documents
- Français pour la documentation principale
- Anglais pour les commentaires de code
- Diagrammes en Mermaid ou ASCII si nécessaire

## 🐛 Signaler un bug

Utiliser le template d'issue avec :
1. Description du problème
2. Étapes pour reproduire
3. Comportement attendu vs observé
4. Environnement (version HA, Pi, etc.)
5. Logs pertinents

## 💡 Proposer une fonctionnalité

1. Vérifier qu'elle n'existe pas déjà dans `docs/FEATURES.md`
2. Créer une issue avec le template "Feature Request"
3. Attendre validation avant de commencer le développement

## 📞 Communication

- **Issues GitHub** : Bugs et features
- **Discussions GitHub** : Questions générales
- **Pull Requests** : Revue de code

## 📜 Licence

En contribuant, vous acceptez que vos contributions soient sous licence Apache 2.0.

---

Merci de contribuer à HA Box ! 🎉
