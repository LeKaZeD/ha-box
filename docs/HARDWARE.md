# Hardware - HA Box

All user-facing peripherals (display, touch, sensors, fan, buzzer, power button) are on the **haboxesp PCB** driven by an **ESP32-WROOM-32**. The Raspberry Pi 5 connects to the ESP32 only via UART0 and two GPIO pins for OTA.

---

## Raspberry Pi 5 stack

| Component | Interface | Notes |
|-----------|-----------|-------|
| Raspberry Pi 5 | — | Main compute; runs Home Assistant OS |
| M.2 SSD hat | PCIe | NVMe storage |
| HDMI + USB-C extension | — | Rear panel port routing |
| Sonoff Zigbee 3.0 USB Dongle Plus E | USB | Zigbee + Thread coordinator |
| CC1101 module | SPI0 (CE0) | 433 MHz radio — RFLink TCP server on :5557 |

### Pi GPIO used by the App

| BCM GPIO | Direction | Connected to | Purpose |
|----------|-----------|-------------|---------|
| 14 (TXD) | Out | ESP32 GPIO 3 (RX0) | UART0 communication |
| 15 (RXD) | In | ESP32 GPIO 1 (TX0) | UART0 communication |
| 23 | Out | ESP32 IO0 | OTA: pull LOW to enter ROM bootloader |
| 24 | Out | ESP32 EN | OTA: pulse LOW → HIGH to reset chip |
| 3.3V (pin 1) | Power | ESP32 GPIO5 (via 1kΩ) | Pi alive signal: HIGH = Pi on, LOW = Pi off |
| 10 (MOSI) | Out | CC1101 MOSI | SPI0 data out |
| 9 (MISO) | In | CC1101 MISO | SPI0 data in |
| 11 (SCK) | Out | CC1101 SCK | SPI0 clock |
| 8 (CE0) | Out | CC1101 CSN | SPI0 chip select |
| 17 | In | CC1101 GDO0 | RX data / interrupt |
| 27 | In | CC1101 GDO2 | Carrier detect / FIFO threshold |

### Pi config.txt (HAOS)

Edit `/mnt/boot/config.txt` (boot partition — not accessible via SSH; use physical access or mount the SD card/NVMe on a computer). The UART must be freed from Bluetooth to expose `/dev/ttyAMA0` to the add-on.

**Raspberry Pi 4** — disables Bluetooth entirely:
```ini
enable_uart=1
dtoverlay=disable-bt
```

**Raspberry Pi 5** — Bluetooth coexists; only `enable_uart=1` is required in most HAOS configurations:
```ini
enable_uart=1
```

For CC1101 433 MHz support (SPI), add on any model:
```ini
dtparam=spi=on
```

After editing, reboot: `ha host reboot`.

---

## haboxesp PCB + ESP32

Custom PCB hosting an ESP32-WROOM-32 with connectors for all peripherals.
KiCad project files: `hardware/haboxesp/`.

### ESP32 peripherals

| Component | Model | Interface | ESP32 pins | Notes |
|-----------|-------|-----------|-----------|-------|
| E-Paper display | GDEY037T03-FT21 (3.7", 240×416) | SPI | See SPI table below | GxEPD2 driver |
| Touch | FT6336U (integrated in display) | I2C | SDA/SCL | Address 0x38 |
| Pi alive | — | GPIO | GPIO5 (VEN pad, via 1kΩ pull-down) | Detects Pi power-off; Pi 3.3V → 1kΩ → GPIO5; `INPUT_PULLDOWN` in firmware |
| Environmental sensor | BME280 | I2C | SDA/SCL | Address 0x76 |
| Case temperature | TMP102 | I2C | SDA/SCL | Address defined in `config.h` |
| Fan | 5 V PWM | PWM | GPIO (config.h) | Temperature-based curve |
| Buzzer | — | GPIO | GPIO (config.h) | 100 ms beep on events |
| Power button | — | GPIO | GPIO 33 (RTC ext0) | Wake from deep sleep |
| J2 connector | — | GPIO | GPIO 26 | Pi power control (pulse = power on/off) |
| UART0 (Pi comm) | — | UART | GPIO 1 (TX) / GPIO 3 (RX) | Pi communication + esptool OTA |
| UART2 (debug) | — | UART | GPIO 17 (TX) / GPIO 16 (RX) | Debug output only; not connected to Pi |

---

## E-Paper display — GDEY037T03-FT21

**Manufacturer**: GooDisplay  
**Controller**: UC8253  
**Datasheet**: `GDEY037T03-FT21.pdf`

| Parameter | Value |
|-----------|-------|
| Size | 3.7" |
| Resolution | 240 × 416 pixels |
| DPI | 130 |
| Active area | 47.04 × 81.54 mm |
| Interface | SPI 4-wire |
| Front-light | 9 LEDs, 2.8 V typical, MOSFET PWM |
| Touch | FT6336U (I2C, address 0x38) |

### SPI wiring (ESP32)

| Signal | ESP32 GPIO |
|--------|-----------|
| CS | GPIO 27 |
| DC | GPIO 14 |
| RST | GPIO 12 |
| BUSY | GPIO 13 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| Colors | 1-bit (black / white) |
| Full refresh | ~2–3 s |
| Partial refresh | ~1 s |
| Standby | 34 µA |
| Deep sleep | 1.1 µA |
| Operating temp | -25 °C to 70 °C |

Front-light: MOSFET default-off (low = off); PWM controls brightness.

---

## BME280 — Environmental sensor

| Parameter | Value |
|-----------|-------|
| Interface | I2C |
| Address | 0x76 (haboxesp default) |
| Temperature | -40 °C to +85 °C, ±1 °C |
| Humidity | 0–100 % RH, ±3 % |
| Pressure | 300–1100 hPa, ±1 hPa |

Polled every 5 s on ESP32; sent to Pi every 30 s via `SENS tC hum pPa`.

---

## TMP102 — Case temperature sensor

| Parameter | Value |
|-----------|-------|
| Interface | I2C |
| Address | Defined in `config.h` |
| Range | -40 °C to +125 °C, ±0.5 °C |

Polled every 5 s by the fan controller; sent to Pi every 30 s via `CASE tC`.

---

## CC1101 — 433 MHz radio module

| Parameter | Value |
|-----------|-------|
| Interface | SPI0, CE0 (GPIO8) |
| Frequency | 433.92 MHz (OOK) |
| GDO0 | GPIO17 (pin 11) — RX data / interrupt |
| GDO2 | GPIO27 (pin 13) — carrier detect / FIFO threshold |
| VCC | 3.3 V (pin 1) |
| GND | GND (pin 6) |

The add-on exposes a **RFLink-compatible TCP server** on `:5557`.
Point the HA RFLink integration (or any RFLink-compatible client) at `localhost:5557`.

**HA configuration (`configuration.yaml`)**:
```yaml
rflink:
  host: localhost
  port: 5557
```

Enable in the add-on options:
```yaml
box:
  rf433:
    enabled: true
    tcp_port: 5557   # default
```

---

## 3D-printed enclosure

Designed in Fusion 360, FDM-printable and modular. Each part can be reprinted independently for hardware upgrades.

**Materials**: PLA body + acrylic window panel + steel backplate.  
**Fasteners**: M2.5 inserts (3.5 mm OD).  
**Printable parts** (STL in `hardware/3D plan/`): `Box`, `Top`, `Backplate`, `Support`, `Pied`, `Button`.  
**Source files**: `hardware/3D plan/Fusion/` (Fusion 360 + STEP).

---

*Last updated: 2026-04-03*
