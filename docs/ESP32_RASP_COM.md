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
| NFC (PN7161, I2C) | ESP32 | UART | ESP32 → Pi | PN7161 on I²C (address `0x24` or `0x48`). ESP32 will send `NFC` messages with UID (and later payload); the App can use them for pairing/onboarding. |
| Onboarding “add device” | HA logic, ESP32 UI | UART | Bidirectional | HA decides the flow; ESP32 displays steps and collects inputs (NFC, touch). Planned, not fully implemented yet. |
| Temperature / humidity / pressure (BME280) | ESP32 | UART | ESP32 → Pi | ESP32 sends `SENS` (tC, hum, pPa); the App publishes values as sensors in Home Assistant. |
| Case temperature (TMP102) | ESP32 | UART | ESP32 → Pi | ESP32 sends `CASE` (tC) every 30 s; the App publishes it as `sensor.ha_box_case_temperature`. |
| Fan PWM (cooling) | ESP32 | UART | Pi → ESP32 | App sends `FAN` (en, tOn, tFull) once on connection; ESP32 applies the curve and persists it in NVS. Fan runs autonomously using TMP102 temperature. |
| Core / Supervisor / Network / Zigbee / Thread / Matter status | Pi / HA (source of truth) | UART | Pi → ESP32 | Implemented: the App sends `STATUS` with key/value fields (`core`, `sup`, `net`, `lan`, `wifi`, `ext`, `zigbee`, `thread`, `matter`); ESP32 maps these to icons. |
| Health check of Apps (Zigbee2MQTT, etc.) | Pi / Supervisor + HA | UART | Pi → ESP32 | The App queries Supervisor and Core entities; results are aggregated into the same `STATUS` payload. |

See `docs/HA_STATUS_TO_ESP32.md` for a deeper description of the status payload and fetchers.

## Hardware wiring

- **UART**: ESP32 uses `Serial2` (the board’s USB debug port is `Serial`).  
  - Pi TXD (GPIO 14) → ESP32 RX2 (GPIO 16)  
  - Pi RXD (GPIO 15) → ESP32 TX2 (GPIO 17)  
  - GND → GND (required for a common reference)  
  - Pi 5 V → ESP32 5 V (if powering the ESP32 from the Pi without USB)

- **NFC (PN7161)**: connected on the ESP32 I²C bus.  
  - I²C address: typically `0x24` or `0x48` (configurable in the App options).  
  - See `docs/HARDWARE.md` and `ha-box/config.yaml` for the exact wiring and configuration.

## UART protocol (summary)

The protocol is ASCII-based, with one message per line:

- Command format:  
  `<id> VERB [key=value ...]`
- ACK format:  
  `ACK <id> OK` or `ACK <id> ERR <code>`

### Verbs from Pi → ESP32

- `READY` – Pi is up and the App is running.
- `HALTED` – Pi / App is shutting down or going offline.
- `SHUTDOWN_ACCEPTED` – The App accepted a shutdown request and asked the Supervisor to stop the host; ESP32 can enter deep sleep immediately.
- `STATUS` – Heartbeat + optional key/values:
  - `core`, `sup`, `net`, `lan`, `wifi`, `ext`, `zigbee`, `thread`, `matter` (all `0` or `1`).
- `WEATHER` – Weather code and outdoor temperature.
- `CLOCK` – Time sync (`hh`, `mm`, `ss`).
- `LANG` – UI language for the ESP32: `id=0` (French), `id=1` (English).
- `FAN` – Fan configuration: `en=0/1`, `tOn=<°C>`, `tFull=<°C>`. Sent once on connection; ESP32 applies immediately and persists in NVS. Values survive deep sleep and are restored on next boot before the Pi reconnects.

### Verbs from ESP32 → Pi

- `READY` – ESP32 is up.
- `SENS` – Ambient sensor payload from BME280:
  - `tC`, `hum`, `pPa`.
- `CASE` – Case (box) temperature from TMP102: `tC`. Sent every 30 s in the same block as `SENS`. The App publishes this as `sensor.ha_box_case_temperature`.
- `STATUS` – Heartbeat when the App does not send a `STATUS` with key/values (legacy / degraded mode).
- `TOUCH` – Touch / button actions (for example navigation, “All Off”).
- `NFC` – NFC events (UID, and later NDEF/metadata).
- `ALL_OFF` – User pressed “All Off” on the E‑Paper UI; the App can call `light.turn_off` on Home Assistant.
- `SHUTDOWN_REQUEST` – User short-pressed the physical power button; the App should trigger a host shutdown.

Protocol implementation details:

- ESP32 side: `esp/ESPHOMEASSISTANT/AsciiProto.*` and `POWERBUTTON.*`.
- App side: `ha-box/rootfs/usr/bin/ha-box/communication/esp32_comm.py` and `message_handler.py`.

## Enabling UART on Home Assistant OS (Pi)

From a root shell on the host (not inside the container), exit the `ha` prompt:

```bash
login
```

Edit `/mnt/boot/config.txt`:

```ini
enable_uart=1
dtoverlay=disable-bt
```

Notes:

- In `vi`, press `i` to enter insert mode, then `Esc`, then `:wq` to save and quit.
- Reboot the host using `ha host reboot` or `reboot`.

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

The following sketch can be used on the ESP32 to test the UART wiring independently from the App:

```cpp
#include <Arduino.h>

// ESP32 UART2 pins
// TX2 = GPIO17, RX2 = GPIO16
static const int UART2_TX = 17;
static const int UART2_RX = 16;

static const uint32_t BAUD_USB  = 115200;
static const uint32_t BAUD_UART = 115200;

String usbLine;
String uartLine;

void setup() {
  Serial.begin(BAUD_USB);

  // UART to Raspberry Pi (3.3 V TTL)
  Serial2.begin(BAUD_UART, SERIAL_8N1, UART2_RX, UART2_TX);

  Serial.println("ESP32 <-> Raspberry bridge ready");
  Serial.println("Type a line in Serial Monitor and press Enter.");
}

static void pumpSerialToUart() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;  // ignore CR
    if (c == '\n') {
      if (usbLine.length() > 0) {
        Serial2.println(usbLine);           // send to Raspberry Pi
        Serial.print("USB -> UART: ");
        Serial.println(usbLine);
        usbLine = "";
      }
    } else {
      usbLine += c;
      if (usbLine.length() > 512) usbLine = ""; // safety guard
    }
  }
}

static void pumpUartToSerial() {
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (uartLine.length() > 0) {
        Serial.print("UART -> USB: ");
        Serial.println(uartLine);
        uartLine = "";
      }
    } else {
      uartLine += c;
      if (uartLine.length() > 512) uartLine = "";
    }
  }
}

void loop() {
  pumpSerialToUart();
  pumpUartToSerial();

  // Optional: heartbeat every 2 s
  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial2.println("ping");
  }
}
```