# Architecture — ESP32 HA Box

## Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         ESP32                                │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────┐      ┌──────────┐      ┌──────────┐          │
│  │  UART0   │ ───> │  Model   │ ───> │    UI    │          │
│  │ (Pi→ESP) │      │ (Shared) │      │ Manager  │          │
│  └──────────┘      └──────────┘      └──────────┘          │
│       │                  │                  │                │
│       │                  │                  ▼                │
│       │                  │         ┌────────────────┐       │
│       │                  │         │  Pages (5)     │       │
│       │                  │         ├────────────────┤       │
│       │                  │         │ - Onboarding   │       │
│       │                  │         │ - Loading      │       │
│       │                  │         │ - Home         │       │
│       │                  │         │ - Settings     │       │
│       │                  │         │ - Shutdown     │       │
│       │                  │         └────────────────┘       │
│       │                  │                  │                │
│       │                  │                  ▼                │
│       │                  │         ┌────────────────┐       │
│       │                  ▼         │ Widgets (11)   │       │
│       │         ┌──────────────┐   ├────────────────┤       │
│       │         │   Sensors    │   │ - StatusIcons  │       │
│       │         │              │   │ - Clock        │       │
│       │         │ - BME280     │   │ - Weather      │       │
│       │         │ - TMP102     │   │ - TempIn       │       │
│       │         │ - PowerBtn   │   │ - DayNight     │       │
│       │         │ - Fan        │   │ - Alerts       │       │
│       │         │ - Touch      │   │ - Button       │       │
│       │         └──────────────┘   │ - Brightness   │       │
│       │                             │ - Sound        │       │
│       │                             │ - Loading      │       │
│       │                             │ - Onboarding   │       │
│       │                             └────────────────┘       │
│       │                                      │                │
│       │                                      ▼                │
│       │                             ┌────────────────┐       │
│       └──────────────────────────> │   E-paper      │       │
│                                     │   240x416      │       │
│                                     │   UC8253       │       │
│                                     └────────────────┘       │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Components

### 1. Model (Shared state)

**Files**: `model/model.h`, `model/model.cpp`

**States**:
- `OnboardingState` — Initial setup (QR code, link)
- `LoadingState` — Loading screen (reason string, animation frame)
- `HomeState` — Dashboard (connectivity flags, weather, temperatures, clock, alerts)
- `SettingsState` — User preferences (brightness 0–4, sound enabled, language 0=FR/1=EN)

**Dirty tracking**:
- Each setter marks dirty flags (`DK_HM_TempIn`, `DK_HM_Clock`, etc.)
- Optimised for e-paper: avoids unnecessary refreshes

### 2. UIManager (Page lifecycle manager)

**File**: `ui/ui_manager.h`

**Responsibilities**:
- Page management (navigation, lifecycle hooks)
- Automatic rendering (full + partial refresh)
- Budget system (max 2 partial refreshes per update cycle)

**Lifecycle**: on navigation — `onExit()` → `onEnter()` → `drawFull()`; each loop — `tick()` → `applyDirty()` → `renderPartials()`.

### 3. Pages

**Files**: `ui/page_*.h`

- **PageOnboarding** — QR code + link for first-time setup
- **PageLoading** — Animated spinner + loading reason string
- **PageHome** — Main dashboard: status icons, clock, weather, temperature, day/night switch, alerts, buttons
- **PageSettings** — User preferences: brightness slider, sound toggle, language selection
- **PageShutdown** — Power-off screen shown during graceful shutdown sequence

**Methods**:
- `onEnter()` — Page init (called on entry)
- `onExit()` — Cleanup (called on exit)
- `tick()` — Local logic (animations, timers)
- `drawFull()` — Full render (full e-paper refresh)
- `applyDirty()` — Mark widgets dirty from model flags
- `renderPartials()` — Partial render (fast e-paper partial refresh)

### 4. Widgets

**Files**: `ui/widgets/widget_*.h`

**List**:
- `StatusIconsWidget` — Row of protocol icons (WiFi, LAN, web, Zigbee, Thread, Matter, 433MHz)
- `ClockWidget` — HH:MM display
- `WeatherWidget` — Weather icon + outdoor temperature
- `TempWidget` — Indoor temperature
- `DayNightSwitchWidget` — Touchable day/night mode toggle
- `AlertsWidget` — Warnings and errors count
- `ButtonWidget` — Touchable button with icon + label (All Off, Settings)
- `BrightnessSliderWidget` — 5-level brightness slider (Settings page)
- `SoundSwitchWidget` — Sound on/off toggle (Settings page)
- `LoadingWidget` — Animated spinner + reason text
- `OnboardingWidget` — QR code + link text

Each widget extends `WidgetBase` (CRTP template): holds a `Rect`, a `dirty` flag, and implements `renderFull()`, `renderPartial()`, and `applyDirty()`.

### 5. UART communication (Pi ↔ ESP32)

**Protocol**: ASCII (`AsciiProto`) — one message per line, ACK-based, deduplication, retry.

**Pi → ESP32**:
```
READY                           → Pi is up and running
HALTED                          → Pi is shutting down
SHUTDOWN_ACCEPTED               → Pi accepted shutdown, ESP32 may sleep
WEATHER code=2 tOut=245         → Weather update (code + outdoor temp in tenths °C)
CLOCK hh=14 mm=32 ss=07         → Clock sync
STATUS core=1 sup=1 wifi=1 ...  → Heartbeat + status icons
DAYMODE mode=0                  → Day/night mode (0=day, 1=night)
LANG id=1                       → Language (0=FR, 1=EN); persisted in NVS
FAN en=1 tOn=28 tFull=60        → Fan configuration; persisted in NVS
VERSION                         → Query firmware version
OTA                             → Firmware flash about to start (show update screen)
```

**ESP32 → Pi**:
```
READY              → ESP32 booted and ready
VERSION ver=0.1.4  → Firmware version (proactive + reply to query)
SENS tC=21.77 hum=48.71 pPa=97759  → BME280 sensor data (every 30 s)
CASE tC=38.2       → TMP102 case temperature (every 30 s)
DAYMODE mode=1     → Manual day/night override from touch switch
SHUTDOWN_REQUEST   → User pressed power button (short press)
ALL_OFF            → User pressed All Off on screen
```

**Callbacks**:
- `authorizeIncoming()` — Whitelist of accepted verbs
- `onMsg()` — Receives message → updates `Model`
- `onAck()` — Receives ACK → resets heartbeat timer

### 6. Sensors & Control

**BME280** (custom driver, `BME280.h`):
- Temperature, humidity, pressure
- I2C (SDA=21, SCL=22)
- Polled every 5 s; model updated directly; aggregated data sent to Pi every 30 s via `SENS`

**TMP102** (custom driver, `TMP102.h`):
- Case (internal) temperature
- I2C; read every 5 s; sent to Pi every 30 s as `CASE`; drives fan PWM curve

**Touch FT6336** (custom driver, `touch_ft6336.h`):
- Capacitive touch controller integrated in the display (I2C, address 0x38)
- Coordinates forwarded to the active page via `uiManager.handleTouch(x, y)`

**PowerButton** (`PowerButton.h`):
- GPIO33 (button input, RTC-capable for deep sleep wake)
- J2 pulse for Pi power control (short pulse = wake/soft-stop, long hold = hard stop)
- Heartbeat timeout: 300 000 ms (5 min) → deep sleep
- Shutdown pending timeout: 60 000 ms → deep sleep fallback

**FanController** (`FanController.h`):
- PWM on GPIO26
- Temperature curve: configurable `tOn` / `tFull` from Pi via `FAN` verb; persisted in NVS
- Safe mode (100% duty) if TMP102 read fails

**i18n** (`i18n.h`, `lang/en.h`, `lang/fr.h`):
- Language selected by the Pi via `LANG id=0|1`; persisted in NVS
- All UI strings accessed through the active language file

**settings_persistence** (`settings_persistence.h`):
- NVS (flash) storage for fan curve, brightness, sound, and language
- Settings survive deep sleep and power cycles

---

## Data flows

### 1. Boot sequence

```
ESP32 boots
  │
  ├─> Init hardware (UART, I2C, Display, Touch, Fan)
  │
  ├─> Restore NVS settings (fan config, brightness, language)
  │
  ├─> UIManager.begin(PageId::Loading)  →  "Waiting for Pi…"
  │
  ├─> Send VERSION to Pi (proactive)
  │
  ├─> Send READY to Pi
  │
  └─> loop()
       │
       ├─> Poll UART → receives READY from Pi
       │                └─> navigateTo(PageId::Home)
       │
       └─> Home page displayed
```

### 2. Sensor data (ESP32 → Pi)

```
loop()
  │
  ├─> BME280.read()  →  tC, hPct, pPa
  │
  ├─> Model.setTempInX10(tC * 10)
  │    └─> dirty |= DK_HM_TempIn
  │
  ├─> proto.sendWithAck("SENS", ...)   (every 30 s)
  │    └─> Pi replies "ACK 2 OK"
  │         └─> touchHeartbeat()
  │
  └─> UIManager.update()
       └─> renderPartials()
            └─> TempWidget partial refresh
```

### 3. Weather data (Pi → ESP32)

```
Pi sends: "5 WEATHER code=2 tOut=245"
  │
  └─> onMsg() callback
       │
       ├─> Model.setWeather(2)      →  dirty |= DK_HM_Weather
       ├─> Model.setTempOutX10(245) →  dirty |= DK_HM_TempOut
       │
       └─> UIManager.update()
            └─> WeatherWidget partial refresh
```

---

## E-paper optimisations

### Full vs Partial refresh

**Full refresh** (slow, ~2 s):
- Page navigation
- Display reset

**Partial refresh** (fast, ~300 ms):
- Widget updates (temperature, clock, etc.)
- Budget: max 2 per update cycle to prevent ghosting

### Dirty tracking

The model marks dirty flags when data changes; the UI consumes the flags and marks only the affected widgets for refresh.

**Flags** (bitmask, defined in `model/dirty_key.h`): `DK_OB_QRPayload`, `DK_OB_Link`, `DK_HM_Status`, `DK_HM_TempIn`, `DK_HM_TempOut`, `DK_HM_Weather`, `DK_HM_Humidity`, `DK_HM_Clock`, and more — one bit per updatable UI element.

### Budget system

`renderPartials()` accepts a budget (default 2). Widgets render in priority order — status icons → clock → weather → temperature — stopping when the budget is exhausted. This prevents multiple slow partial refreshes from blocking the main loop.

---

## Main loop

See `main.cpp` for the full implementation. In summary: `proto.poll()` → sensor reads → send SENS/CASE to Pi every 30 s → `uiManager.update()`.
