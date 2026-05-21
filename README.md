# HA Box

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

**HA Box** is an open-source, modular home automation hub built around a Raspberry Pi 5 and Home Assistant OS. It integrates the main smart home protocols (Zigbee, Thread, 433 MHz) into a single 3D-printed enclosure, with a custom ESP32 interface board providing a local E-Paper display, touch, environmental sensing, and physical controls.

---

## Project overview

HA Box is composed of four independent layers that work together:

```
+---------------------------------------------------------------+
|  3D-printed enclosure  (modular, FDM-printable, upgradeable)  |
|                                                               |
|  +---------------------------+   +-------------------------+  |
|  |  Raspberry Pi 5 stack     |   |  haboxesp PCB + ESP32   |  |
|  |  - Pi 5                   |   |  - E-Paper display      |  |
|  |  - M.2 SSD hat            |   |  - Touch (FT6336U)      |  |
|  |  - HDMI/USB-C extension   |   |  - BME280 sensor        |  |
|  |  - Sonoff Dongle Plus E   |   |  - Temp sensor TMP102   |  |
|  |    (Zigbee + Thread)      |   |  - Fan (PWM)            |  |
|  |  - CC1101 module (planned)|   |  - Buzzer               |  |
|  |    (433 MHz, SPI)         |   |  - Power button + J2    |  |
|  |                           |   |                         |  |
|  |  [ Home Assistant OS ]    |   |                         |  |
|  |  [ HA Box App (py) ]   <--+---+--> UART                 |  |
|  +---------------------------+   +-------------------------+  |
+---------------------------------------------------------------+
```

| Layer | Description | Location |
|-------|-------------|----------|
| **Hardware** | Pi 5 stack, haboxesp PCB, Sonoff dongle, connectors | `hardware/` |
| **3D enclosure** | Modular, printable enclosure for all components | `hardware/3D plan/` |
| **ESP32 firmware** | Drives display, sensors, touch, fan, buzzer | `ha-box-esp/` |
| **HA App** | Python app: UART bridge to HA API and Supervisor | `ha-box/` |
| **HA integration** | Custom component: groups all HA Box entities in the device registry | `ha-integration/` |

---

## Hardware

### Raspberry Pi 5 stack

| Component | Description | Protocol |
|-----------|-------------|----------|
| Raspberry Pi 5 | Main compute unit, runs Home Assistant OS | — |
| M.2 SSD hat | NVMe storage via PCIe | PCIe |
| HDMI + USB-C extension | Rear panel port routing | — |
| Sonoff Zigbee 3.0 USB Dongle Plus E | Zigbee and Thread coordinator | USB |
| CC1101 module | 433 MHz radio (planned) | SPI |

### haboxesp PCB + ESP32

The haboxesp is a custom PCB designed for this project. It hosts an ESP32 module and provides connectors for all user-facing peripherals. The ESP32 communicates with the Pi exclusively over UART.

| Component | Model | Interface |
|-----------|-------|-----------|
| E-Paper display | GDEY037T03-FT21 (3.7", 240×416) | SPI |
| Touch controller | FT6336U (integrated in display) | I2C |
| Environmental sensor | BME280 (temperature, humidity, pressure) | I2C |
| Case temperature | Additional sensor | I2C / analog |
| Fan | 5V PWM | PWM |
| Buzzer | Haptic feedback | GPIO |
| Power button | Wake + shutdown trigger | GPIO |
| J2 connector | Raspberry Pi J2 header (power control) | — |

PCB design files (KiCad) and manufacturing exports are in `hardware/haboxesp/`.

### 3D-printed enclosure

The enclosure is designed in Fusion 360, modular and FDM-printable. Each part can be reprinted independently to accommodate hardware upgrades.

Printable parts (STL): `Box`, `Top`, `Backplate`, `Support`, `Pied`, `Button`.

| | |
|---|---|
| ![Front](hardware/3D%20plan/images/Home_assistant_box_Face.png) | ![Inside](hardware/3D%20plan/images/Home_assistant_box_inside.png) |

Fusion 360 source and STEP files: `hardware/3D plan/Fusion/`.

---

## ESP32 firmware

The ESP32 firmware (`ha-box-esp-fw/`) is a C++ application that:

- drives the E-Paper display (SPI) and front-light
- reads touch input (I2C, FT6336U)
- reads temperature/humidity/pressure (I2C, BME280)
- controls the fan (PWM, temperature-based)
- manages the power button (wake from deep sleep, shutdown request)
- controls the buzzer for haptic feedback
- communicates with the Pi App over UART using a line-based ASCII protocol

See [ha-box-esp-fw/ESPHOMEASSISTANT/ARCHITECTURE.md](ha-box-esp-fw/ESPHOMEASSISTANT/ARCHITECTURE.md) and [docs/ESP32_RASP_COM.md](docs/ESP32_RASP_COM.md) for the firmware architecture and UART protocol.

---

## HA App

The HA Box App (`ha-box/`) is a Home Assistant OS Supervisor App written in Python. It runs inside a Docker container on the Pi and acts as a bridge between the ESP32 (UART) and Home Assistant (HTTP API + Supervisor API).

It does not access display, sensors, or actuators directly. All peripherals are on the ESP32 side; the App communicates over UART0 and controls two Pi GPIOs (IO0 + EN) for OTA firmware updates.

### Raspberry Pi configuration (UART)

Edit `/mnt/boot/config.txt` physically (keyboard + screen on the Pi, or SD card / NVMe mounted on a computer). See [docs/HARDWARE.md](docs/HARDWARE.md#pi-configtxt-haos) for Pi 4 vs Pi 5 differences and SPI (CC1101) setup. Reboot after: `ha host reboot`.

### Installation

1. Add this repository to your Home Assistant Apps:

   [![Add repository](https://my.home-assistant.io/badges/supervisor_add_addon_repository.svg)](https://my.home-assistant.io/redirect/supervisor_add_addon_repository/?repository_url=https%3A%2F%2Fgithub.com%2FLeKaZeD%2Fha-box)

   Or manually: **Settings** → **App** → **App Store** → **⋮** → **Repositories** → add the URL.

2. Install **HA Box**.
3. Install the **HA Box custom integration**: copy `ha-integration/custom_components/ha_box/` to your HA `config/custom_components/` folder, then restart Home Assistant. This creates the HA Box device in the device registry.
4. Configure options according to your hardware (see below).
5. Start the App.

### Configuration

Configure options in the App UI or directly in YAML. Restart the App to apply changes.

```yaml
general:
  log_level: "info"                         # debug | info | warning | error | critical
  lang: "auto"                              # auto | fr | en

weather:
  weather_entity: "weather.forecast_home"   # HA weather entity
  update_interval: 300                      # seconds (10–3600)

bme280:
  enabled: true
  address: "auto"                           # auto | 0x76 | 0x77
  update_interval: 30                       # seconds

fan:
  enabled: false
  min_temp: 28                              # °C — fan starts
  max_temp: 60                              # °C — fan at full speed

ota:
  io0_gpio: 23                              # Pi BCM GPIO → ESP32 IO0
  en_gpio: 24                               # Pi BCM GPIO → ESP32 EN

daynight:
  update_interval: 60                       # seconds between sun.sun polls (10–3600)

clock:
  update_interval: 60                       # seconds (10–3600)

status:
  update_interval: 30                       # seconds
  exposed_addon_slugs: []                   # slugs that activate the "exposed" icon

uart:
  device: "auto"                            # auto | /dev/serial0 | /dev/ttyAMA0 | /dev/ttyUSB0

connection:
  connected_timeout: 120                    # seconds before switching to reconnecting
  reconnecting_timeout: 60                  # seconds before switching to disconnected
```

Language is auto-detected from the Home Assistant Core configuration (`/api/config` → `language`), with fallback to the container environment (`LANG`) and then English.

Fan and OTA options are sent to the ESP32 on every App startup and persisted in NVS — restart the App to apply changes.

---

## Current status (v0.2.0)

| Feature | Interface | Status |
|---------|-----------|--------|
| E-Paper display (Home, Loading, Settings pages) | SPI (ESP32) | Implemented |
| Touch navigation | I2C (ESP32) | Implemented |
| BME280 (temp, humidity, pressure) → HA entities | UART (ESP32 → Pi) | Implemented |
| TMP102 case temperature → HA entity | UART (ESP32 → Pi) | Implemented |
| Fan control (PWM, temperature-based, configurable) | PWM (ESP32) | Implemented |
| Power button (wake, soft shutdown, hard J2 stop) | UART + J2 (ESP32 ↔ Pi) | Implemented |
| Weather + clock display | UART (Pi → ESP32) | Implemented |
| HA status icons (Core, Supervisor, Net, Zigbee, Thread, Matter, External) | UART (Pi → ESP32) | Implemented |
| OTA firmware update (auto version check + flash over UART0) | UART0 + GPIO | Implemented |
| Boot failure recovery (blind OTA flash on ROM boot errors) | UART0 + GPIO | Implemented |
| Day/Night mode sync (sun.sun + manual override, HA event + binary sensor) | UART (Pi ↔ ESP32) | Implemented |
| HA Box integration (device registry, grouped entities) | HA custom component | In progress |
| CC1101 433 MHz | SPI (Pi) | Planned |

---

## Future: Compute Module board

The long-term vision is a custom carrier board built around a Raspberry Pi Compute Module, integrating the Pi, ESP32, radio modules, and all connectors on a single PCB. This would replace the current assembly of off-the-shelf boards and the haboxesp PCB with a more compact, reliable, and user-friendly product.

Early design work is in `hardware/cm5/`.

---

## Repository structure

```
ha-box/
├── hardware/
│   ├── haboxesp/          KiCad PCB project (haboxesp V0.1)
│   ├── 3D plan/           Fusion 360 enclosure + STL parts
│   └── cm5/               Compute Module carrier board (early stage)
├── ha-box-esp/         ESP32 firmware (C++)
├── ha-box/                HA App (Python)
│   ├── rootfs/            App runtime files
│   ├── config.yaml        App manifest
│   └── translations/      UI translations (en, fr)
├── ha-integration/        HA custom integration (device registry, grouped entities)
│   └── custom_components/
│       └── ha_box/
├── docs/                  Technical documentation
└── README.md
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | App technical architecture |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Hardware specifications (haboxesp PCB + ESP32) |
| [docs/ESP32_RASP_COM.md](docs/ESP32_RASP_COM.md) | UART protocol (Pi ↔ ESP32) |
| [docs/FEATURES_STATUS.md](docs/FEATURES_STATUS.md) | Feature status and timing reference |
| [docs/TECH_STACK.md](docs/TECH_STACK.md) | Libraries and technical stack |
| [docs/I18N.md](docs/I18N.md) | Multilingual support |
| [ROADMAP.md](ROADMAP.md) | Current status and future plans |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution guide |
| [ha-box/SECURITY.md](ha-box/SECURITY.md) | Security policy |
| [TESTING.md](TESTING.md) | Development and testing guide |

---

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) to get started.

---

## License

Apache License 2.0. See [LICENSE](LICENSE).

---

*Project started January 17, 2026*
