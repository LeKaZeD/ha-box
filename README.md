# HA Box

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
![Supports aarch64 Architecture](https://img.shields.io/badge/aarch64-yes-green.svg)

**HA Box** is an open-source, modular home automation hub built around a Raspberry Pi 5 and Home Assistant OS. It integrates the main smart home protocols (Zigbee, Thread, 433 MHz) into a single 3D-printed enclosure, with a custom ESP32 interface board providing a local E-Paper display, touch, NFC, environmental sensing, and physical controls.

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
|  |  - HDMI/USB-C extension   |   |  - NFC (PN7161)         |  |
|  |  - Sonoff Dongle Plus E   |   |  - BME280 sensor        |  |
|  |    (Zigbee + Thread)      |   |  - Temp sensor TMP102   |  |
|  |  - CC1101 module (planned)|   |  - Fan (PWM)            |  |
|  |    (433 MHz, SPI)         |   |  - Buzzer               |  |
|  |                           |   |  - Power button + J2    |  |
|  |  [ Home Assistant OS ]    |   |                         |  |
|  |  [ HA Box add-on (py) ]<--+---+--> UART                 |  |
|  +---------------------------+   +-------------------------+  |
+---------------------------------------------------------------+
```

| Layer | Description | Location |
|-------|-------------|----------|
| **Hardware** | Pi 5 stack, haboxesp PCB, Sonoff dongle, connectors | `hardware/` |
| **3D enclosure** | Modular, printable enclosure for all components | `hardware/3D plan/` |
| **ESP32 firmware** | Drives display, sensors, touch, NFC, fan, buzzer | `ha-box-esp-fw/` |
| **HA add-on** | Python app: UART bridge to HA API and Supervisor | `ha-box/` |

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
| NFC controller | PN7161 (NXP) | I2C |
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
- reads NFC tags (I2C, PN7161)
- reads temperature/humidity/pressure (I2C, BME280)
- controls the fan (PWM, temperature-based)
- manages the power button (wake from deep sleep, shutdown request)
- controls the buzzer for haptic feedback
- communicates with the Pi add-on over UART using a line-based ASCII protocol

See [ha-box-esp-fw/ESPHOMEASSISTANT/ARCHITECTURE.md](ha-box-esp-fw/ESPHOMEASSISTANT/ARCHITECTURE.md) and [docs/ESP32_RASP_COM.md](docs/ESP32_RASP_COM.md) for the firmware architecture and UART protocol.

---

## HA add-on

The HA Box add-on (`ha-box/`) is a Home Assistant OS Supervisor App written in Python. It runs inside a Docker container on the Pi and acts as a bridge between the ESP32 (UART) and Home Assistant (HTTP API + Supervisor API).

It does not access any hardware directly. All sensors and actuators are on the ESP32 side; the add-on sends and receives messages over UART.

### Raspberry Pi configuration (UART)

Enable UART and disable Bluetooth on Home Assistant OS by editing `/mnt/boot/config.txt`:

```ini
enable_uart=1
dtoverlay=disable-bt
```

Reboot the host after the change. See [docs/ESP32_RASP_COM.md](docs/ESP32_RASP_COM.md) for detailed steps.

### Installation

1. Add this repository to your Home Assistant Apps:

   [![Add repository](https://my.home-assistant.io/badges/supervisor_add_addon_repository.svg)](https://my.home-assistant.io/redirect/supervisor_add_addon_repository/?repository_url=https%3A%2F%2Fgithub.com%2FLeKaZeD%2Fha-box)

   Or manually: **Settings** → **Add-ons** → **Add-on Store** → **⋮** → **Repositories** → add the URL.

2. Install **HA Box**.
3. Configure options according to your hardware (see below).
4. Start the add-on.

### Configuration

```yaml
home_assistant:
  weather_entity: "weather.forecast_home"
  update_interval: 300

display:
  enabled: true
  rotation: 0
  refresh_mode: "full"
  front_light:
    enabled: true
    brightness: 50

sensors:
  bme280:
    enabled: true
    address: "auto"
    update_interval: 30
  nfc:
    enabled: true
    address: "0x24"
    polling_interval: 1
  touch:
    enabled: true

control:
  fan:
    enabled: false
    pin: 18
    auto_control: true
    min_temp: 40
    max_temp: 60
  led:
    enabled: false
    pin: 21
    count: 10
```

Language is auto-detected from the Home Assistant Core configuration (`/api/config` → `language`), with fallback to the container environment (`LANG`) and then English.

---

## Current status

| Feature | Interface | Status |
|---------|-----------|--------|
| E-Paper display | SPI (ESP32) | Implemented (alpha) |
| Touch interface | I2C (ESP32) | Implemented (basic UI) |
| Power button + J2 shutdown | UART (ESP32 ↔ Pi) | Implemented |
| BME280 sensor | I2C (ESP32) | Implemented |
| Fan control | PWM (ESP32) | Implemented (temperature-based) |
| HA status icons (Core, Supervisor, Net, Zigbee, Thread, Matter) | UART | Implemented |
| NFC reader (PN7161) | I2C (ESP32) | Planned |
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
├── ha-box-esp-fw/         ESP32 firmware (C++)
├── ha-box/                HA add-on (Python)
│   ├── rootfs/            Add-on runtime files
│   ├── config.yaml        Add-on manifest
│   └── translations/      UI translations (en, fr)
├── docs/                  Technical documentation
└── README.md
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Add-on technical architecture |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Detailed hardware specifications |
| [docs/ESP32_RASP_COM.md](docs/ESP32_RASP_COM.md) | UART protocol (Pi ↔ ESP32) |
| [docs/FEATURES.md](docs/FEATURES.md) | Features specification |
| [docs/TECH_STACK.md](docs/TECH_STACK.md) | Technical stack |
| [docs/I18N.md](docs/I18N.md) | Multilingual support |
| [ROADMAP.md](ROADMAP.md) | Development roadmap |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution guide |
| [ha-box/SECURITY.md](ha-box/SECURITY.md) | Security policy |
| [TESTING.md](TESTING.md) | Testing and installation guide |

---

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) to get started.

---

## License

Apache License 2.0. See [LICENSE](LICENSE).

---

*Project started January 17, 2026*
