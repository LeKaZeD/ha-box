# HA Box — Documentation

## Prerequisites

- Raspberry Pi 4 or 5 running Home Assistant OS
- haboxesp board connected via UART (`/dev/serial0`)
- `config.txt` configured (see below)

### Configuring /mnt/boot/config.txt

This file is on the boot partition and **cannot be edited via SSH or the web terminal**. Access it physically — either by connecting a keyboard and screen to the Pi, or by mounting the SD card / NVMe on a computer and editing `config.txt` at the root of the boot partition.

**Raspberry Pi 4** — disables Bluetooth to free `/dev/ttyAMA0`:
```ini
enable_uart=1
dtoverlay=disable-bt
```

**Raspberry Pi 5** — Bluetooth coexists; only `enable_uart=1` is needed:
```ini
enable_uart=1
```

**CC1101 433 MHz** (any model) — also add:
```ini
dtparam=spi=on
```

After editing: `ha host reboot`

---

## Installation

1. Add this repository to Home Assistant Apps (Settings → Apps → App Store → ⋮ → Repositories)
2. Install **HA Box**
3. Install the **HA Box custom integration**: copy `ha-integration/custom_components/ha_box/` to your HA `config/custom_components/` folder, then restart Home Assistant
4. Configure options (see below)
5. Start the app

---

## Configuration

Restart the app to apply changes.

```yaml
general:
  log_level: "info"       # trace | debug | info | warning | error | critical
  lang: auto              # auto | fr | en

weather:
  weather_entity: "weather.forecast_home"
  update_interval: 300    # seconds (10–300)

bme280:
  enabled: true
  address: "auto"         # auto | 0x76 | 0x77
  update_interval: 30     # seconds (10–300)

fan:
  enabled: true
  min_temp: 28            # °C — fan starts
  max_temp: 60            # °C — fan at full speed

ota:
  io0_gpio: 23            # Pi BCM GPIO → ESP32 IO0
  en_gpio: 24             # Pi BCM GPIO → ESP32 EN
  force_flash: false      # flash firmware on every startup regardless of version

daynight:
  update_interval: 60     # seconds (10–3600)

clock:
  update_interval: 60     # seconds (10–3600)

status:
  update_interval: 30     # seconds (10–300)
  exposed_addon_slugs: [] # App slugs to show as "exposed" in status bar

uart:
  device: "auto"          # auto | /dev/serial0 | /dev/ttyAMA0 | /dev/ttyUSB0

connection:
  connected_timeout: 120  # seconds before CONNECTED → RECONNECTING (30–600)
  reconnecting_timeout: 60 # seconds before RECONNECTING → DISCONNECTED (10–300)

rf433:
  enabled: false          # enable CC1101 433 MHz module
  tcp_port: 5557
  spi_bus: 0
  spi_device: 0
  gdo0_pin: 17            # CC1101 GDO0 → Pi GPIO17
  gdo2_pin: 27            # CC1101 GDO2 → Pi GPIO27
```

### Language

Auto-detected from Home Assistant Core (`Settings → System → General`), with fallback to the container `LANG` environment variable, then English.

### 433 MHz RF (CC1101)

When `rf433.enabled: true`, the app exposes a RFLink-compatible TCP server on port 5557. Point the HA RFLink integration at the Pi's local IP:

```yaml
# configuration.yaml
rflink:
  host: 192.168.1.x   # Pi local IP
  port: 5557
```

CC1101 wiring (Pi GPIO header):

| CC1101 pin | Pi GPIO | Physical pin |
|------------|---------|--------------|
| MOSI | GPIO10 | 19 |
| MISO | GPIO9 | 21 |
| SCK | GPIO11 | 23 |
| CSN | GPIO8 (CE0) | 24 |
| GDO0 | GPIO17 | 11 |
| GDO2 | GPIO27 | 13 |
| VCC | 3.3 V | 1 |
| GND | GND | 6 |

---

## Troubleshooting

**UART not detected** — Check that `enable_uart=1` is in `config.txt` and the App option `uart.device` is set to `auto`.

**ESP32 firmware not updating** — Verify `ota.io0_gpio` and `ota.en_gpio` match the wiring (default: GPIO23 / GPIO24). Check App logs for esptool output.

**No sensor entities in HA** — Ensure the `ha_box` custom integration is installed and Home Assistant was restarted after copying it.
