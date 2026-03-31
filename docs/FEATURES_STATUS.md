# Features status

What HA Box does, organized by responsibility.
Update status when a feature changes: `done` | `partial` | `planned`
`optional` = not required for basic operation.
`[not reviewed]` = behavior not verified in code; check before optimizing.

---

## ESP32 only

| Feature | Status | When | UART from Pi | UART to Pi | Notes / to review |
|---------|--------|------|-------------|-----------|-------------------|
| Display the time (clock, local auto-advance) | done | Continuous (~1 s local); synced on `CLOCK` receipt | `CLOCK hh mm ss` | — | Clock interval hardcoded to 60 s on Pi side |
| Display indoor temperature | done | On BME280 read (every 5 s) | — | — | Read directly from sensor on ESP32 |
| Display indoor humidity | done | On BME280 read (every 5 s) | — | — | Not yet shown on screen |
| Display indoor pressure | done | On BME280 read (every 5 s) | — | — | BME280 data present; widget not wired |
| Monitor case temperature (TMP102) | done | Polled every 5 s (fan loop) | — | — | Drives fan PWM; always compiled in |
| Send BME280 data to Pi | done | Every 30 s (`SENS_PERIOD_MS`) | — | `SENS tC hum pPa` | BME280 polled every 5 s; aggregated and sent every 30 s |
| Send case temperature to Pi | done | Every 30 s (same block as SENS) | — | `CASE tC` | TMP102 read on-demand in SENS block; separate verb from SENS |
| Fan control (temperature-based PWM) | done | Continuous (~100 ms loop) | `FAN en tOn tFull` | — | Curve configurable at runtime via FAN verb from Pi; defaults: off < tOn-3 °C, ramp tOn–tFull, full > tFull |
| **Button: short click, Pi OFF** → start Pi | done | On event (120 ms – 2 s press) | — | — | `pulseJ2()` = 150 ms pulse on J2; `setPiState(ON)` |
| **Button: short click, Pi ON** → request shutdown | done | On event | — | `SHUTDOWN_REQUEST` | Sets state SHUTDOWN_PENDING |
| **Button: long press (≥ 5 s), Pi ON** → J2 pulse | done | On event | — | — | Calls `pulseJ2()` (150 ms). `resetJ2()` (5500 ms hard stop) exists but **never called**; if intent is forced shutdown, `resetJ2()` should be used here |
| **Button: long press, Pi OFF** | partial | On event | — | — | No action (`handleLongPress()` has no `else` for Pi OFF). Intentional? |
| Button locked during SHUTDOWN_PENDING | done | While state = SHUTDOWN_PENDING | — | — | Click and long press ignored until state changes |
| Deep sleep on heartbeat timeout | done | 5 min after last UART activity | — | — | `hbTimeoutMs = 300 000` ms in `.ino`; default in `.h` is 5000 ms — discrepancy |
| Deep sleep fallback (SHUTDOWN_PENDING) | done | 60 s after SHUTDOWN_REQUEST if no reply | — | — | `shutdownPendingTimeoutMs = 60 000` ms |
| Deep sleep on SHUTDOWN_ACCEPTED / HALTED | done | On event (UART) | `SHUTDOWN_ACCEPTED` or `HALTED` | — | Any UART activity also resets heartbeat timer |
| Wake from deep sleep on button press | done | On event (ext0, GPIO33) | — | — | GPIO33 is RTC-capable; verified in code |
| Buzzer feedback | done | On event | — | — | 100 ms beep; exact trigger points [not reviewed] |
| Switch language (FR / EN) | done | On `LANG` receipt | `LANG id=0/1` | — | Persisted in NVS; triggers UI rebuild |
| Read NFC tags | planned | On event (tag detection) | — | `NFC uid=...` (planned) | PN7161 driver not written |

---

## Pi (add-on) only

| Feature | Status | When | UART from ESP32 | UART to ESP32 | Notes / to review |
|---------|--------|------|----------------|--------------|-------------------|
| Detect HA Core health | done | Every 30 s | — | — | GET /core/api |
| Detect Supervisor health | done | Every 30 s | — | — | GET /supervisor/info |
| Detect LAN / WiFi / internet connectivity | done | Every 30 s | — | — | Derived from network interfaces |
| Detect Zigbee active | done | Every 30 s | — | — | ZHA entities or Zigbee2MQTT add-on slug |
| Detect Thread active | done | Every 30 s | — | — | OTBR / OpenThread add-on slug |
| Detect Matter active | done | Every 30 s | — | — | Matter add-on slug or entities |
| Detect external access – Cloudflare tunnel | done | Every 30 s | — | — | Trusts Supervisor state (started = tunnel up); no outbound HTTP |
| Detect external access – DuckDNS | done | Every 30 s | — | — | Add-on started + URL reachability check |
| Expose BME280 temperature to HA | done | On `SENS` receipt (~30 s) | `SENS tC hum pPa` | — | `sensor.ha_box_temperature` |
| Expose BME280 humidity to HA | done | On `SENS` receipt (~30 s) | `SENS tC hum pPa` | — | `sensor.ha_box_humidity` |
| Expose BME280 pressure to HA | done | On `SENS` receipt (~30 s) | `SENS tC hum pPa` | — | `sensor.ha_box_pressure` (hPa) |
| Expose case temperature (TMP102) to HA | done | On `CASE` receipt (~30 s) | `CASE tC` | — | `sensor.ha_box_case_temperature` |
| Forward NFC tag UID to HA | planned | On event | `NFC uid=...` (planned) | — | Depends on NFC ESP32 feature |
| Shut down host on SHUTDOWN_REQUEST | done | On event | `SHUTDOWN_REQUEST` | `SHUTDOWN_ACCEPTED` | Calls Supervisor /host/shutdown |
| Auto-detect HA language | done | Once at add-on startup | — | — | GET /core/api/config → language field |

---

## ESP32 + Pi together

| Feature | Status | When | Notes / to review |
|---------|--------|------|-------------------|
| Display the time (synced from Pi) | done | Once on connection + every 60 s | Clock interval hardcoded to 60 s in `main.py` |
| Display weather + outdoor temperature | done | Once on connection + every 300 s | Interval configurable (10–300 s) |
| Display HA status icons (9 icons) | done | Once on connection + every 30 s | STATUS interval hardcoded to 30 s in `main.py` |
| Apply display language from HA config | done | Once on connection | Language sent via `LANG id=0/1` on first connect; re-sent on reconnect |
| Apply fan config from HA add-on options | done | Once on connection (re-sent on reconnect) | `FAN en=0/1 tOn=X tFull=Y` sent after LANG; persisted in ESP32 NVS so values survive deep sleep |
| Shut down Pi from button (soft) | done | On event | Short click → SHUTDOWN_REQUEST → Pi → /host/shutdown → SHUTDOWN_ACCEPTED → deep sleep |
| Force-stop Pi from button (hard J2 hold) | partial | On event (long press) | `resetJ2()` (5500 ms) exists but never called; `pulseJ2()` (150 ms) used instead |
| Wake Pi from button | done | On event (short click, Pi OFF) | pulseJ2 on boot and on button press when Pi is OFF |
| Notify ESP32 when Pi is ready | done | Once on Pi startup / reconnect | |
| Notify ESP32 when Pi is halting | done | On event (system halt) | |

---

## Configuration – App (add-on)

Config key path is relative to `options:` in `config.yaml`.
Language is auto-detected from Home Assistant Core at startup (not a user option).

| Option | Config key | Default | Notes |
|--------|-----------|---------|-------|
| Weather entity | `home_assistant.weather_entity` | `weather.forecast_home` | HA entity for weather + outdoor temperature |
| Weather update interval | `home_assistant.update_interval` | 300 s | 10–300 s |
| Display rotation | `display.rotation` | 0 | 0–3; used by ESP32 display driver |
| **Fan enabled** | `esp32.fan.enabled` | false | Sent to ESP32 as `FAN en=0/1`; persisted in ESP32 NVS |
| **Fan min temperature** | `esp32.fan.min_temp` | 28 °C | `tOn`: fan starts; sent as `FAN tOn=X`; persisted in NVS |
| **Fan max temperature** | `esp32.fan.max_temp` | 60 °C | `tFull`: fan at full speed; sent as `FAN tFull=Y`; persisted in NVS |

> **Note:** `esp32.*` options are sent to the ESP32 once at add-on startup and persisted in NVS.
> The ESP32 restores these values from NVS on every boot, even before the Pi reconnects.
> To apply a change, restart the add-on.

---

## Configuration – ESP32 firmware

Compile-time constants in `config.h` or `.ino`. Require reflashing to change.

| Parameter | Value | Notes |
|-----------|-------|-------|
| BME280 I2C address | 0x76 | |
| TMP102 I2C address | `TMP102_ADDR` (define) | |
| Touch FT6336U I2C address | 0x38 | |
| Fan off temperature (default) | tOn − 3 °C | Hysteresis; derived from `tOn` in `setCurve()` |
| Fan on temperature (default) | 28 °C | Overridden at runtime by `FAN tOn=X` from Pi |
| Fan full speed temperature (default) | 60 °C | Overridden at runtime by `FAN tFull=Y` from Pi |
| J2 short pulse duration | 150 ms (`j2PulseMs`) | Used for wake + short-press soft stop |
| J2 long pulse duration | 5500 ms (`j2ResetMs`) | Hard stop; `resetJ2()` exists but never called |
| Heartbeat timeout (Pi ON) | 300 000 ms (5 min) | Set in `.ino`; default in `.h` is 5000 ms |
| Shutdown pending timeout | 60 000 ms (60 s) | Fallback if SHUTDOWN_ACCEPTED not received |
| Button short click range | 120 ms – 2 s | Presses between 2 s – 5 s are ignored |
| Button long press threshold | ≥ 5 s | |
| SENS send interval | 30 000 ms (30 s) | |
| CASE send interval | 30 000 ms (30 s) | Same block as SENS |
| BME280 internal poll interval | 5 000 ms (5 s) | |
