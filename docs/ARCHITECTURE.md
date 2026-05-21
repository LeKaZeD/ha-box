# Technical Architecture - HA Box

This document describes the technical architecture of the HA Box App and its link to the ESP32 firmware.

## Overview

The HA Box App runs on Home Assistant OS (Raspberry Pi) inside a Supervisor-managed container. It does **not** drive display, sensors, or actuators directly on the Pi. Instead, it talks to an **ESP32** over **UART** (serial). The ESP32 runs the E-Paper display, touch, BME280, fan, and power-button logic; the App provides Home Assistant integration (Supervisor/Core API), status aggregation, and data push/pull over the ASCII protocol.

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Home Assistant OS (Raspberry Pi)                  │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                         Supervisor                                │  │
│  │  ┌─────────────────────────────────────────────────────────────┐  │  │
│  │  │              HA Box App.  (Container)                       │  │  │
│  │  │                                                              │  │  │
│  │  │  ┌─────────────┐  ┌──────────────────────────────────────┐  │  │  │
│  │  │  │   Handlers  │  │  Communication (UART)                │  │  │  │
│  │  │  │  Weather    │  │  esp32_comm, message_handler,         │  │  │  │
│  │  │  │  Clock      │  │  connection_manager                    │  │  │  │
│  │  │  │  Status     │  └──────────────────┬───────────────────┘  │  │  │
│  │  │  │  Sensor     │                       │                      │  │  │
│  │  │  └──────┬─────┘                       │                      │  │  │
│  │  │         │                             │                      │  │  │
│  │  │  ┌──────┴─────────────────────────────┴──────────────────┐  │  │  │
│  │  │  │  API (Supervisor + Core): config, states, services,    │  │  │  │
│  │  │  │  host shutdown, status fetchers                        │  │  │  │
│  │  │  └────────────────────────────┬───────────────────────────┘  │  │  │
│  │  └───────────────────────────────┼──────────────────────────────┘  │  │
│  └──────────────────────────────────┼──────────────────────────────────┘  │
│                                     │                                      │
│  ┌──────────────────────────────────┼──────────────────────────────────┐  │
│  │  Linux kernel                     │   /dev/serial0, /dev/ttyAMA0      │  │
│  └──────────────────────────────────┼──────────────────────────────────┘  │
└─────────────────────────────────────┼───────────────────────────────────────┘
                                      │ UART (115200 8N1)
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ESP32                                           │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  AsciiProto (UART)  ◄──────────────────────────────────────────────►  │  │
│  │  PowerButton (GPIO btn + J2), Model, UI (Loading / Home / Settings)   │  │
│  └───────┬───────────────────────────────────────────────────────────────┘  │
│          │                                                                  │
│  ┌───────┴───────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌─────┐  ┌──────┐  │
│  │ E-Paper (SPI) │  │ Touch  │  │ BME280 │  │ Fan │  │ J2   │  │
│  │ GDEY037T03    │  │ FT6336 │  │ I2C    │  │ PWM │  │Power │  │
│  └───────────────┘  └────────┘  └────────┘  └────────┘  └─────┘  └──────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

See [docs/ESP32_RASP_COM.md](ESP32_RASP_COM.md) for the UART protocol and responsibilities split between Pi and ESP32.

## Main Components (App)

### 1. API Layer (`api/ha_api.py`)

Client for the Supervisor API and Home Assistant Core API.

**Responsibilities:**
- Supervisor: info, network, addons list/info, host shutdown
- Core: config (e.g. language), entity states, services (e.g. `light.turn_off`)
- Aggregation of system status (core, supervisor, network, Zigbee/Thread/Matter via Apps and Core entities) for the STATUS message to the ESP32

**Authentication:** `SUPERVISOR_TOKEN` and `SUPERVISOR_URL` from the environment.

### 2. Communication (`communication/`)

UART link to the ESP32.

**Components:**
- **esp32_comm**: Serial open/close, line framing, parsing of ASCII protocol (`<id> VERB [key=val ...]` and `ACK <id> OK|ERR`), send commands, optional send-with-ACK, background poll thread; detects ROM boot failure patterns (`boot_failure_count`)
- **message_handler**: Routes incoming messages (READY, VERSION, SENS, CASE, STATUS, TOUCH, ALL_OFF, SHUTDOWN_REQUEST); authorizes verbs; triggers host shutdown and sends SHUTDOWN_ACCEPTED; stores `esp32_firmware_version`
- **connection_manager**: State machine (CONNECTING, CONNECTED, RECONNECTING, DISCONNECTED); periodic READY when not connected; relies on StatusHandler for STATUS when connected
- **ota_manager**: Compares bundled firmware version (`ha-box/firmware/VERSION`) with ESP32 reported version; controls IO0/EN GPIOs via `gpiod`; invokes `esptool.main()` in-process to flash `0x10000`; handles blind-flash recovery on boot failure

### 3. Handlers (`handlers/`)

**SensorHandler:** Receives SENS (tC, hum, pPa) and CASE (tC) from the ESP32, fires `ha_box_sensors` and `ha_box_case_temp` events on the HA event bus. The HA Box integration listens to these events and updates entities in the device registry.

**WeatherHandler:** Fetches weather entity from Core, sends WEATHER (code, tOut) to the ESP32 on a timer when connected.

**ClockHandler:** Sends CLOCK (hh, mm, ss) to the ESP32 on a timer when connected.

**StatusHandler:** Uses status fetchers (core, supervisor, network, addons for Zigbee/Thread/Matter, etc.) to build a status dict and sends STATUS with key/values to the ESP32 on a timer when connected. Acts as the heartbeat in CONNECTED state.

**DayNightHandler:** Polls sun.sun periodically and sends DAYMODE (mode=0/1) to the ESP32. Handles manual override when the user touches the day/night switch on the ESP32 (DAYMODE message from ESP32); the override is cleared on the next real sun event. Fires `ha_box_mode_changed` event on every mode change.

### 4. Core (`core/`)

- **config**: Load options from `SUPERVISOR_OPTIONS` (e.g. `/data/options.json`), build `Config` with all `box.*` sections (weather, bme280, fan, ota, daynight, clock, status, uart, connection)
- **i18n**: Translator; language from HA Core config when available, else env (LANG, SUPERVISOR_LANGUAGE), else default English
- **logger**: Logging setup

The App does **not** include a Hardware Abstraction Layer (HAL) for I2C/SPI/GPIO on the Pi. All display, touch, BME280, fan, and power-button handling run on the **ESP32**. Pin and bus details are in [docs/HARDWARE.md](HARDWARE.md) and the ESP32 firmware (e.g. `esp/ESPHOMEASSISTANT/`).

## Communication with Home Assistant

### Supervisor and Core API

The App communicates with the Supervisor and Home Assistant Core via HTTP/REST using the Supervisor proxy:

```
┌─────────────┐     HTTP/REST      ┌─────────────┐
│   HA Box    │ ◄────────────────► │  Supervisor │
│    App      │  SUPERVISOR_TOKEN  │     API     │
└─────────────┘                    └──────┬──────┘
                                           │
                                    ┌──────┴──────┐
                                    │ Core API    │
                                    │ (e.g. /core/api/config, states, services)
                                    └─────────────┘
```

### Endpoints used

| Endpoint | Usage |
|----------|--------|
| `/core/api/config` | Read Core config (e.g. language for i18n) |
| `/core/api/states` | Read entity states (weather, sun.sun) |
| `/core/api/events/<type>` | Fire events (`ha_box_sensors`, `ha_box_case_temp`, `ha_box_mode_changed`) |
| `/core/api/services/light/turn_off` | Call services (e.g. all lights off) |
| `/host/shutdown` | Request host shutdown (power button short press) |
| `/supervisor/info` | Supervisor status |
| `/network/info` | Network interfaces |
| `/addons`, `/addons/<slug>/info` | App list and options (status, Zigbee/Thread/Matter detection) |

### UART link to ESP32

The App talks to the ESP32 over a serial device (`/dev/serial0` or `/dev/ttyAMA0`) at 115200 8N1. Protocol: ASCII lines `<id> VERB [key=value ...]` with ACK lines `ACK <id> OK|ERR <code>`. See [docs/ESP32_RASP_COM.md](ESP32_RASP_COM.md) for the verb list and responsibilities.

## File structure (App)

```
ha-box/
├── config.yaml           # App configuration (options, schema, devices)
├── build.yaml            # Build configuration (if used)
├── Dockerfile            # Docker image
├── apparmor.txt          # AppArmor profile (when enabled)
├── CHANGELOG.md
├── README.md
├── icon.png
├── logo.png
├── translations/
│   ├── en.yaml
│   └── fr.yaml
└── rootfs/
    ├── etc/
    │   └── services.d/
    │       └── ha-box/
    │           ├── run       # Startup script (s6)
    │           └── finish    # Shutdown script (s6)
    └── usr/
        └── bin/
            └── ha-box/
                ├── main.py              # Entry point
                ├── core/
                │   ├── __init__.py
                │   ├── config.py        # Options, Config
                │   ├── i18n.py          # Translator, language detection
                │   └── logger.py
                ├── api/
                │   ├── __init__.py
                │   └── ha_api.py        # Supervisor + Core API client
                ├── communication/
                │   ├── __init__.py
                │   ├── esp32_comm.py    # UART, protocol, ACK, boot failure detection
                │   ├── message_handler.py
                │   ├── connection_manager.py
                │   └── ota_manager.py   # OTA flash via esptool (GPIO + UART0)
                └── handlers/
                    ├── __init__.py
                    ├── sensor_handler.py
                    ├── weather_handler.py
                    ├── clock_handler.py
                    ├── status_handler.py
                    ├── daynight_handler.py
                    └── status_fetchers/
                        ├── __init__.py
                        ├── core.py
                        ├── supervisor.py
                        ├── network.py
                        ├── addons.py
                        ├── zigbee.py
                        ├── thread.py
                        └── matter.py
```

## Required hardware configuration

### Raspberry Pi (HAOS)

The App uses only **UART** to talk to the ESP32. No I2C, SPI, or GPIO are required on the Pi for the App.

Edit `/mnt/boot/config.txt` per [docs/HARDWARE.md](HARDWARE.md#pi-configtxt-haos), then reboot. The serial device will appear as `/dev/serial0` or `/dev/ttyAMA0`. See [docs/ESP32_RASP_COM.md](ESP32_RASP_COM.md) for wiring and OTA GPIO pin details.

### ESP32 (firmware)

Display (SPI), touch (I2C), BME280 (I2C), fan (PWM), and power button (GPIO + J2) are on the **ESP32** side. Pin assignments, I2C addresses, and bus layout are documented in [docs/HARDWARE.md](HARDWARE.md). Summary:

| Device | Interface | Address / pin (ESP32) | Notes |
|--------|-----------|------------------------|-------|
| Touch (FT6336U) | I2C | 0x38 | Integrated in GDEY037T03-FT21 |
| BME280 | I2C | 0x76 or 0x77 | Configurable in App options |
| E-Paper | SPI | CS, DC, RST, BUSY, etc. | See HARDWARE.md and firmware |
| Fan | PWM | Configurable (e.g. GPIO 18) | Local control on ESP32 |
| Power button | GPIO | e.g. GPIO 33 | ext0 wake; J2 pulse (e.g. GPIO 26) for Pi power |

## Startup sequence

```
┌────────────────────────────────────────────────────────────────┐
│ 1. Supervisor starts the container                              │
├────────────────────────────────────────────────────────────────┤
│ 2. s6-overlay runs the 'run' script                             │
├────────────────────────────────────────────────────────────────┤
│ 3. main.py: load options, build Config                          │
├────────────────────────────────────────────────────────────────┤
│ 4. Initialize HA API client (Supervisor URL, token)             │
├────────────────────────────────────────────────────────────────┤
│ 5. Language: get_core_language() or env fallback, get_translator│
├────────────────────────────────────────────────────────────────┤
│ 6. Handlers: SensorHandler, WeatherHandler, ClockHandler,       │
│    StatusHandler; ESP32 comm and connection_manager             │
├────────────────────────────────────────────────────────────────┤
│ 7. Open serial port, start poll thread, set callbacks            │
├────────────────────────────────────────────────────────────────┤
│ 8. Initialization loop: send READY periodically until           │
│    connection_manager.is_connected() (recent message from ESP32) │
│    — if ESP32 sends ROM boot errors (>= 3), trigger blind OTA   │
│      flash (GPIO IO0/EN + esptool) without waiting for VERSION  │
├────────────────────────────────────────────────────────────────┤
│ 9. OTA check: query VERSION from ESP32 (up to 3 retries);       │
│    if bundled version differs, flash via esptool; restart loop  │
├────────────────────────────────────────────────────────────────┤
│10. Once connected: send LANG + FAN config, force               │
│    weather/clock/status update, start periodic timers          │
├────────────────────────────────────────────────────────────────┤
│11. Main loop: connection_manager.update(), send READY/STATUS   │
│    as needed; sensor_handler.update_home_assistant() from SENS  │
│    If connection lost, return to step 8                         │
└────────────────────────────────────────────────────────────────┘
```

## Error handling

### Fallback strategy

| Error | Behavior |
|-------|----------|
| Core not available at startup | Language falls back to env or default; API calls (e.g. weather, config) fail until Core is up; handlers retry or skip |
| Serial port missing or open failure | App logs error and exits or runs without ESP32 (depending on startup path) |
| ESP32 disconnection (no recent message) | connection_manager moves to RECONNECTING then DISCONNECTED; READY sent periodically; when message received again, CONNECTED and services resume |
| Host shutdown request fails (e.g. 403) | Log warning; ESP32 does not receive SHUTDOWN_ACCEPTED and may use shutdownPendingTimeoutMs then deep sleep |
| Sensor/weather/status API failure | Handlers log and skip or use cached data; next cycle retries |

### Logging

Use `bashio::log.*` for bash scripts and Python `logging` module:

```python
import logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("ha-box")
```

Levels:
- `DEBUG`: Technical details
- `INFO`: Normal events
- `WARNING`: Non-blocking issues
- `ERROR`: Recoverable errors
- `CRITICAL`: Fatal errors

## Security

### Permissions (current)

The App uses serial devices for the ESP32 link and GPIO for OTA:

```yaml
devices:
  - /dev/ttyAMA0
  - /dev/serial0
  - /dev/gpiochip0
gpio: true
```

`gpio: true` and `/dev/gpiochip0` are required for the OTA manager to control the IO0 and EN pins. I2C and SPI are not used on the Pi. If AppArmor is enabled, the profile must allow access to the chosen serial device(s) and to Supervisor/network as used by the API client. See [ha-box/SECURITY.md](../ha-box/SECURITY.md) and [docs/HA_APPS_COMPLIANCE.md](HA_APPS_COMPLIANCE.md).

---

## Multilingual support (i18n)

- **Translation files**: `ha-box/translations/{language}.yaml` (e.g. en, fr) for configuration UI labels/descriptions (used by Home Assistant).
- **Python**: `core/i18n.py` provides a `Translator` and `get_translator()`. Language is chosen in this order: explicit from HA Core config (`/core/api/config` → `language`), then `LANG` or `SUPERVISOR_LANGUAGE` from the environment, then default English.
- **ESP32**: The App sends `LANG` with `id=0` (French) or `id=1` (English) so the firmware can show text in the correct language.

See [docs/I18N.md](I18N.md) for full details if present.

## HA Box integration (custom component)

A companion HA integration (`ha-integration/custom_components/ha_box/`) listens to events fired by the App and exposes all entities under a single **HA Box** device in the HA device registry:

| Event | Entities created |
|-------|-----------------|
| `ha_box_sensors` | Temperature, Humidity, Pressure |
| `ha_box_case_temp` | Case Temperature |
| `ha_box_mode_changed` | Mode (Jour / Nuit) |

Install by copying `custom_components/ha_box/` to your HA config folder, restarting HA, then adding the integration via Settings → Devices & Services → Add Integration → HA Box.

## Future evolutions

- Publish integration to HACS
- Additional languages (e.g. DE, ES) in translations and LANG id mapping

---

*Last updated: 2026-04-02*
