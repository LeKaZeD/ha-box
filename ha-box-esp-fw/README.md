# HA Box – ESP32 Firmware

Firmware for the `haboxesp` PCB (ESP32-WROOM-32).  
Handles the E-Paper display, touch, sensors, fan, buzzer, and UART link to the Raspberry Pi.

---

## Prerequisites

### Arduino IDE setup

1. Install **Arduino IDE 2.x**
2. Add the ESP32 board package:  
   `File > Preferences > Additional boards manager URLs`  
   → `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. `Tools > Board > Boards Manager` → install **esp32 by Espressif Systems**

### Board settings

| Setting | Value |
|---------|-------|
| Board | `ESP32 Dev Module` |
| Upload Speed | `115200` |
| Partition Scheme | `Default 4MB with spiffs` |
| Port | your USB-Serial port |

### Libraries (Library Manager)

| Library | Author |
|---------|--------|
| `GxEPD2` | ZinggJM |

Everything else (`Wire`, `SPI`, `Preferences`, `driver/rtc_io`) is bundled with the Espressif package.

---

## ⚠️ Missing file — required before compiling

The display driver header is not in the repository:

```
ESPHOMEASSISTANT/gdey/GxEPD2_370_GDEY037T03.h
```

Create the `gdey/` folder inside `ESPHOMEASSISTANT/` and place the driver file there.  
It is a custom driver for the **GDEY037T03-FT21** (3.7″, 240×416).  
Without it the build fails immediately at the first `#include`.

---

## Build & flash

Open `ESPHOMEASSISTANT/ESPHOMEASSISTANT.ino` in Arduino IDE and click **Upload**.

The firmware uses two serial ports:

| Port | Pins | Purpose |
|------|------|---------|
| `Serial` (UART0) | USB / GPIO1+3 | Debug output (Serial Monitor) |
| `Serial2` (UART2) | GPIO16 (RX) / GPIO17 (TX) | UART link to Raspberry Pi |

---

## Serial console commands (debug)

With the Serial Monitor open at **115200 baud**:

| Command | Action |
|---------|--------|
| `t` | Read TMP102 case temperature |
| `f<0-4>` | Set front-light level (0 = off, 4 = max) |
| `v<0-255>` | Set fan PWM duty manually |
| `l<0/1>` | Set language (0 = FR, 1 = EN) |
| `j` | Force all status icons ON (UI test) |
| `w<0-15>` | Set weather icon (UI test) |
| `p` / `o` / `m` | Navigate to Onboarding / Home / Loading page |

---

## OTA update from Home Assistant (planned)

The goal is to flash the ESP32 from the HA add-on without physical access, using the existing UART connection and the Pi's GPIO.

### Hardware requirements

The `haboxesp` PCB exposes two ESP32 pins to the Raspberry Pi:

| ESP32 pin | Role | Pi side |
|-----------|------|---------|
| `IO0` (GPIO0) | BOOT mode — pull LOW before reset to enter bootloader | Pi GPIO (output) |
| `EN` | Reset — pulse LOW then HIGH to restart the chip | Pi GPIO (output) |

UART0 (GPIO1/3) is used for both debug today and flashing in bootloader mode — same physical lines.

### Update flow

```
1. Add-on sends a clean shutdown command to the ESP32 (UART2)
2. Pi pulls IO0 LOW
3. Pi pulses EN LOW → HIGH  (ESP32 restarts in bootloader mode)
4. Add-on runs esptool.py on UART0 with the new .bin
5. Pi releases IO0 HIGH
6. Pi pulses EN again  (ESP32 restarts on new firmware)
7. Add-on reads VERSION from ESP32, confirms update
```

### Getting the .bin

Arduino IDE does not produce a `.bin` automatically on Upload.  
To export it explicitly:

```
Sketch > Export Compiled Binary
```

This creates `ESPHOMEASSISTANT.ino.bin` in the sketch folder.  
That file is then committed to the repository under `ha-box-esp-fw/firmware/` and bundled with every add-on release. The add-on always carries the matching firmware version for itself.

### Add-on side (HA integration)

The add-on update (via HA Supervisor) and the ESP32 firmware update are two separate things, but they are linked:

- When the add-on is updated, it may carry a new `.bin`
- On startup, the add-on reads the ESP32 firmware version via a `VERSION` UART verb and compares it with the bundled version
- **If versions match** → nothing happens
- **If versions differ** → two possible behaviors (configurable):
  - **Auto (default):** flash immediately on startup, no user action required
  - **Manual:** expose a HA `update` entity; user clicks **Install** in `Settings > Updates`

The auto mode is the simplest for end users (update the add-on → ESP32 updates itself on next boot). The manual mode is safer if a failed flash is a concern.

### Current status

Not implemented. Blocked on:
- Wiring `IO0` and `EN` to Pi GPIO confirmed ✓ (pins exposed on PCB)
- UART0 shared between debug and bootloader — needs to switch role during update
- `esptool.py` integration in the add-on
- `VERSION` UART verb on ESP32 side
- `.bin` export and storage convention (`ha-box-esp-fw/firmware/`)
