# ESP32 ↔ Raspberry Pi UART Link

This document describes how the ESP32 firmware and the Raspberry Pi (running Home Assistant OS and the HA Box App) communicate over UART, and which responsibilities live on each side.

## High-level responsibilities

| Function | Owner (side) | Link (ESP32 ↔ Pi) | Direction | Notes |
|---------|--------------|--------------------|-----------|-------|
| Front “Power” push button | ESP32 reads button | **J2 (power button)** | ESP32 → Pi | ESP32 simulates a power-button press on J2 (pulse ~150 ms). Short press = 120 ms–2 s, long press ≥ 5 s. |
| Graceful shutdown (PC-like) | Pi / HAOS | **UART** (short press) + **J2** (long press) | ESP32 → Pi | **Short press**: ESP32 sends `SHUTDOWN_REQUEST` → App calls Supervisor host shutdown API (see HA docs) → App sends `SHUTDOWN_ACCEPTED` → ESP32 enters deep sleep. **Long press**: ESP32 pulses J2 for a hard shutdown. |
| Wake / power on (without cutting power) | Pi wakes | **J2 (power button)** | ESP32 → Pi | ESP32 is woken by `ext0` on the button, then `powerBtn.begin()` triggers `pulseJ2()` to power on the Pi. |
| Reboot / maintenance | Pi / HAOS | UART | ESP32 → Pi | Reserved for future: ESP32 sends a whitelisted command (for example `REBOOT`), App triggers `ha host reboot` or equivalent. Not implemented yet. |
| E‑Paper display (SPI) | ESP32 drives display | UART | Pi → ESP32 | Pi sends **data** (`WEATHER`, `CLOCK`, `STATUS`, `LANG`, …). ESP32 updates the model and renders the UI pages (Loading, Home, Settings). |
| Touch (I2C) | ESP32 reads touch | UART | ESP32 → Pi | ESP32 sends `TOUCH` and high‑level actions (Home / All Off / Settings). The App can convert these into automations or service calls. |
| Onboarding “add device” | HA logic, ESP32 UI | UART | Bidirectional | HA decides the flow; ESP32 displays steps and collects inputs (touch). Planned, not fully implemented yet. |
| Temperature / humidity / pressure (BME280) | ESP32 | UART | ESP32 → Pi | ESP32 sends `SENS` (tC, hum, pPa); the App publishes values as sensors in Home Assistant. |
| Case temperature (TMP102) | ESP32 | UART | ESP32 → Pi | ESP32 sends `CASE` (tC) every 30 s; the App publishes it as `sensor.ha_box_case_temperature`. |
| Fan PWM (cooling) | ESP32 | UART | Pi → ESP32 | App sends `FAN` (en, tOn, tFull) once on connection; ESP32 applies the curve and persists it in NVS. Fan runs autonomously using TMP102 temperature. |
| Core / Supervisor / Network / Zigbee / Thread / Matter status | Pi / HA (source of truth) | UART | Pi → ESP32 | Implemented: the App sends `STATUS` with key/value fields (`core`, `sup`, `net`, `lan`, `wifi`, `ext`, `zigbee`, `thread`, `matter`); ESP32 maps these to icons. |
| Health check of Apps (Zigbee2MQTT, etc.) | Pi / Supervisor + HA | UART | Pi → ESP32 | The App queries Supervisor and Core entities; results are aggregated into the same `STATUS` payload. |

See `docs/HA_STATUS_TO_ESP32.md` for a deeper description of the status payload and fetchers.

## Hardware wiring

- **UART0** (Pi <-> ESP32 communication + OTA flashing):
  - Pi TXD (BCM 14) -> ESP32 RX0 (GPIO 3)
  - Pi RXD (BCM 15) -> ESP32 TX0 (GPIO 1)
  - GND -> GND (required for a common reference)

- **OTA GPIO** (bootloader entry, wired on haboxesp PCB):
  - Pi BCM 23 -> ESP32 IO0 (pull LOW before reset to enter ROM bootloader)
  - Pi BCM 24 -> ESP32 EN (pulse LOW then HIGH to reset the chip)

- **UART2** (ESP32 debug output only — not connected to Pi):
  - GPIO 17 (TX2) / GPIO 16 (RX2) — use a USB-serial adapter for debug if needed

## UART protocol (summary)

The protocol is ASCII-based, with one message per line:

- Command format:  
  `<id> VERB [key=value ...]`
- ACK format:  
  `ACK <id> OK` or `ACK <id> ERR <code>`

### Verbs from Pi -> ESP32

- `READY` - Pi is up and the App is running.
- `HALTED` - Pi / App is shutting down or going offline.
- `SHUTDOWN_ACCEPTED` - The App accepted a shutdown request; ESP32 can enter deep sleep immediately.
- `STATUS` - Heartbeat + key/values: `core`, `sup`, `net`, `lan`, `wifi`, `ext`, `zigbee`, `thread`, `matter`, `mhz433` (all `0` or `1`); `warn` and `err` (integers — HA warning/error counts).
- `WEATHER` - Weather code and outdoor temperature.
- `CLOCK` - Time sync (`hh`, `mm`, `ss`).
- `LANG` - UI language: `id=0` (French), `id=1` (English).
- `FAN` - Fan configuration: `en=0/1`, `tOn=<C>`, `tFull=<C>`. Sent once on connection; ESP32 persists in NVS.
- `DAYMODE` - Day/night mode: `mode=0` (day) or `mode=1` (night). Sent on sun.sun state change; also sent by ESP32 on manual override (bidirectional).
- `OTA` - Announces that a firmware flash is about to start. ESP32 switches to the firmware-update loading screen. Sent just before esptool begins flashing.
- `VERSION` - Query ESP32 firmware version. ESP32 replies with `VERSION ver=X.Y.Z`.

### Verbs from ESP32 -> Pi

- `READY` - ESP32 booted and is ready. Sent once after boot (after a 1 s settling delay).
- `VERSION ver=X.Y.Z` - Firmware version. Sent proactively after `READY`; also sent in reply to a Pi `VERSION` query. Used by the add-on for OTA version comparison.
- `SENS` - BME280 ambient sensor: `tC`, `hum`, `pPa`. Sent every 30 s.
- `CASE` - TMP102 case temperature: `tC`. Sent every 30 s. Published as `sensor.ha_box_case_temperature`.
- `STATUS` - Heartbeat when App does not send `STATUS` (legacy / degraded mode).
- `DAYMODE` - Manual day/night override from the E-Paper touch switch: `mode=0` (day) or `mode=1` (night). The add-on applies the override and clears it on the next real sun event.
- `TOUCH` - Touch / button actions (e.g. navigation, All Off).
- `ALL_OFF` - User pressed All Off on E-Paper UI; App calls `light.turn_off` on HA.
- `SHUTDOWN_REQUEST` - User short-pressed the physical power button; App triggers host shutdown.

Protocol implementation details:

- ESP32 side: `esp/ESPHOMEASSISTANT/AsciiProto.*` and `POWERBUTTON.*`.
- App side: `ha-box/rootfs/usr/bin/ha-box/communication/esp32_comm.py` and `message_handler.py`.

## Enabling UART on Home Assistant OS (Pi)

From a root shell on the host (not inside the container), exit the `ha` prompt:

```bash
login
```

Edit `/mnt/boot/config.txt` (see [docs/HARDWARE.md](HARDWARE.md#pi-configtxt-haos) for Pi 4 vs Pi 5 differences), then reboot: `ha host reboot`.

Check that UART is enabled:

```bash
ls -l /dev/serial*
```

If you see `/dev/serial0` or `/dev/ttyAMA0`, the UART device is available.

For **manual tests only** (without the App), you can configure the serial port:

```bash
stty -F /dev/serial0 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo
```

When the HA Box App is running, it opens the serial device itself at 115200 8N1; you do not need to run `stty` manually.

Quick manual test:

- Read: `cat /dev/serial0`  
- Write: `echo "hello from haos" > /dev/serial0`

## Example UART test sketch for ESP32

This sketch bridges UART0 (Pi link) to the USB Serial Monitor for manual testing.
It uses UART2 (GPIO16/17) for the Arduino USB debug port so UART0 is free for the Pi.

```cpp
#include <Arduino.h>

// UART0: Pi communication (GPIO1=TX, GPIO3=RX) — same port used by esptool for OTA
// UART2: Debug / Serial Monitor (GPIO17=TX, GPIO16=RX)
static const int DEBUG_TX = 17;
static const int DEBUG_RX = 16;
static const uint32_t BAUD = 115200;

String uartLine;
String debugLine;

void setup() {
  // UART0 -> Pi (3.3 V TTL)
  Serial.begin(BAUD);
  // UART2 -> USB Serial Monitor
  Serial2.begin(BAUD, SERIAL_8N1, DEBUG_RX, DEBUG_TX);
  Serial2.println("ESP32 <-> Raspberry bridge ready");
}

static void pumpPiToDebug() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '') continue;
    if (c == '
') {
      if (uartLine.length() > 0) {
        Serial2.print("Pi -> Debug: ");
        Serial2.println(uartLine);
        uartLine = "";
      }
    } else {
      uartLine += c;
      if (uartLine.length() > 512) uartLine = "";
    }
  }
}

static void pumpDebugToPi() {
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '') continue;
    if (c == '
') {
      if (debugLine.length() > 0) {
        Serial.println(debugLine);  // send to Pi
        Serial2.print("Debug -> Pi: ");
        Serial2.println(debugLine);
        debugLine = "";
      }
    } else {
      debugLine += c;
      if (debugLine.length() > 512) debugLine = "";
    }
  }
}

void loop() {
  pumpPiToDebug();
  pumpDebugToPi();
}
```
