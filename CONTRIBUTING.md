# Contribution Guide - HA Box

Welcome! This document describes the rules and process for contributing to the HA Box project.

## Prerequisites

### Required Knowledge

- **Home Assistant OS**: Understanding of Apps and their configuration
- **Embedded Linux**: Basic knowledge of I2C, SPI, GPIO buses
- **Docker**: Containerization basics
- **Bash/Python**: Depending on components to develop

### Recommended Hardware

- Raspberry Pi 4 or 5 with Home Assistant OS
- haboxesp PCB with ESP32-WROOM-32
- GDEY037T03-FT21 E-Paper display (with FT6336U touch)
- BME280 sensor (on ESP32 I2C bus)
- 5V PWM fan

### Development Environment

- Git configured
- SSH access to your Home Assistant instance
- IDE of your choice (VSCode recommended)
- Docker for local tests (optional)

## Git Workflow

### Branches

| Branch | Usage |
|--------|-------|
| `main` | Stable version, releases only |
| `develop` | Active development, integration |
| `feature/*` | New features |
| `fix/*` | Bug fixes |
| `docs/*` | Documentation only |

### Contribution Process

1. **Fork** the repository (external contributors)
2. **Create a branch** from `develop`:
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/ma-fonctionnalite
   ```
3. **Develop** following conventions
4. **Test** on real hardware if possible
5. **Commit** with clear messages
6. **Push** and create a **Pull Request** to `develop`

### Commit Messages

**All commit messages must be written in English.**

Format: `type(scope): description`

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `refactor`: Refactoring
- `test`: Test addition/modification
- `chore`: Maintenance

Examples:
```
feat(display): add ILI9341 display support
fix(uart): fix SENS read timeout
docs(readme): update installation
```

## Code Conventions

### Bash

- Use `#!/usr/bin/with-contenv bashio` for s6 scripts
- Indentation: 2 spaces
- Variable names: `UPPER_SNAKE_CASE` for constants, `lower_snake_case` for variables
- Always quote variables: `"${variable}"`
- Use `shellcheck` for validation

### Python

- Python 3.9+ minimum
- Style: PEP 8
- Use `black` for formatting
- Use `pylint` or `flake8` for validation
- Type hints required for public functions and classes

### YAML

- Indentation: 2 spaces
- No tabs
- Explanatory comments for complex options

## Security and Permissions

### Principle of Least Privilege

- Request only necessary permissions
- Document each required permission and why
- Use AppArmor when possible

### Hardware Devices

In `config.yaml`, explicitly declare:
```yaml
devices:
  - /dev/ttyAMA0
  - /dev/serial0
  - /dev/gpiochip0
gpio: true
```

### Protection Mode

- By default, keep Protection Mode enabled
- If deactivation necessary, document reasons

## Tests

### Required Tests

1. **Unit tests**: For any business logic
2. **Integration tests**: Communication with devices
3. **Manual tests**: On real hardware before PR

### Checklist Before PR

- [ ] Code follows conventions
- [ ] Tests pass
- [ ] Documentation updated
- [ ] Tested on hardware (if applicable)
- [ ] No credentials/secrets in code
- [ ] `ha-box/CHANGELOG.md` updated (if observable behavior changed)

## Documentation

### Files to Maintain

| File | Content |
|------|---------|
| `README.md` | Project overview |
| `ROADMAP.md` | Current status and planned work |
| `docs/FEATURES_STATUS.md` | Feature status and timing (code-aligned) |
| `docs/ARCHITECTURE.md` | Technical architecture |
| `CONTRIBUTING.md` | This file |
| `ha-box/CHANGELOG.md` | App version history |

### Documentation Standards

- Markdown for all documents
- English for main documentation
- English for code comments
- Mermaid or ASCII diagrams if necessary

## Reporting Bugs

Use the issue template with:
1. Problem description
2. Steps to reproduce
3. Expected vs observed behavior
4. Environment (HA version, Pi, etc.)
5. Relevant logs

## Proposing a Feature

1. Verify it doesn't already exist in `ROADMAP.md` or `docs/FEATURES_STATUS.md`
2. Create an issue with "Feature Request" template
3. Wait for validation before starting development

## Communication

- **GitHub Issues**: Bugs and features
- **GitHub Discussions**: General questions
- **Pull Requests**: Code review

## License

By contributing, you agree that your contributions are licensed under Apache 2.0.

---

Thank you for contributing to HA Box!
