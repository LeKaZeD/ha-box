# Architecture ESP32 HA Box

## Vue d'ensemble

```
┌─────────────────────────────────────────────────────────────┐
│                         ESP32                                │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────┐      ┌──────────┐      ┌──────────┐          │
│  │  UART2   │ ───> │  Model   │ ───> │    UI    │          │
│  │ (Pi→ESP) │      │ (Shared) │      │ Manager  │          │
│  └──────────┘      └──────────┘      └──────────┘          │
│       │                  │                  │                │
│       │                  │                  ▼                │
│       │                  │         ┌────────────────┐       │
│       │                  │         │  Pages (3)     │       │
│       │                  │         ├────────────────┤       │
│       │                  │         │ - Onboarding   │       │
│       │                  │         │ - Loading      │       │
│       │                  │         │ - Home         │       │
│       │                  │         └────────────────┘       │
│       │                  │                  │                │
│       │                  │                  ▼                │
│       │                  │         ┌────────────────┐       │
│       │                  ▼         │ Widgets (7)    │       │
│       │         ┌──────────────┐   ├────────────────┤       │
│       │         │   Sensors    │   │ - StatusIcons  │       │
│       │         │              │   │ - Clock        │       │
│       │         │ - BME280     │   │ - Weather      │       │
│       │         │ - PowerBtn   │   │ - TempIn       │       │
│       │         │ - Fan        │   │ - Loading      │       │
│       │         └──────────────┘   │ - Onboarding   │       │
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

## Composants

### 1. Model (État partagé)

**Fichiers** : `model/model.h`, `model/model.cpp`

**États** :
- `OnboardingState` : Configuration initiale (QR code, lien WiFi)
- `LoadingState` : Écran de chargement (raison, animation)
- `HomeState` : Dashboard (WiFi, BLE, météo, temp, horloge, batterie)

**Dirty tracking** :
- Chaque setter marque des flags dirty (`DK_HM_TempIn`, `DK_HM_Clock`, etc.)
- Optimisé pour e-paper (éviter refresh inutiles)

### 2. UIManager (Gestionnaire de cycle de vie)

**Fichier** : `ui/ui_manager.h`

**Responsabilités** :
- Gestion des pages (navigation, lifecycle)
- Render automatique (full + partial refresh)
- Budget system (max 2 partial refresh par update)

**Lifecycle** :
```cpp
navigateTo(PageId::Home)
  └─> onExit()  → onEnter()  → drawFull()

update()
  └─> tick()  → applyDirty()  → renderPartials()
```

### 3. Pages

**Fichiers** : `ui/page_*.h`

- **PageOnboarding** : QR code + lien pour config WiFi
- **PageLoading** : Animation + raison de chargement
- **PageHome** : Dashboard avec 4 widgets

**Méthodes** :
- `onEnter()` : Init page (appelé à l'entrée)
- `onExit()` : Cleanup (appelé à la sortie)
- `tick()` : Logique locale (animations, timers)
- `drawFull()` : Render complet (full refresh)
- `applyDirty()` : Marquer widgets dirty
- `renderPartials()` : Render partiel (partial refresh)

### 4. Widgets

**Fichiers** : `ui/widgets/widget_*.h`

**Liste** :
- `StatusIconsWidget` : WiFi + BLE status
- `ClockWidget` : Heure:Minute
- `WeatherWidget` : Icône + temp ext + humidité
- `TempWidget` : Température intérieure
- `LoadingWidget` : Raison + animation dots
- `OnboardingWidget` : QR code + lien

**Pattern** :
```cpp
template<typename Derived>
struct WidgetBase {
  Rect r;           // Zone d'affichage
  bool dirty;       // Flag dirty
  
  renderFull()      // Render sans partial window
  renderPartial()   // Render avec partial window (optimisé)
  applyDirty()      // Check dirty flags du model
}
```

### 5. Communication UART (Pi ↔ ESP32)

**Protocole** : ASCII (`AsciiProto`)

**Pi → ESP32** :
```
1 READY              → Pi est démarré
2 WEATHER code=2 tOut=245 hum=65  → Météo
3 CLOCK hh=14 mm=32  → Horloge
4 STATUS             → Heartbeat
```

**ESP32 → Pi** :
```
1 READY              → ESP32 est démarré
2 SENS tC=21.77 hum=48.71 pPa=97759  → Capteur BME280
3 SHUTDOWN_REQUEST   → Demande shutdown
```

**Callbacks** :
- `authorizeIncoming()` : Whitelist des commandes
- `onMsg()` : Reçoit message → met à jour `Model`
- `onAck()` : Reçoit ACK → touch heartbeat

### 6. Sensors & Control

**BME280** :
- Température, humidité, pression
- I2C (SDA=21, SCL=22)
- Lecture toutes les secondes

**PowerButton** :
- GPIO33 (bouton)
- GPIO26 (pulse J2 pour Pi)
- Heartbeat timeout : 180 secondes
- Deep sleep si Pi OFF

**FanController** :
- PWM GPIO25
- Contrôle ventilateur selon température
- Safe mode si erreur BME280

## Flux de données

### 1. Boot séquence

```
ESP32 boot
  │
  ├─> Init hardware (UART, I2C, Display, Touch)
  │
  ├─> UIManager.begin(PageId::Loading)
  │     └─> "Waiting for Pi..."
  │
  ├─> Send "READY" to Pi via UART
  │
  └─> Loop()
       │
       ├─> Poll UART → Reçoit "READY" de Pi
       │                └─> Model.setWifiBle(true)
       │                     └─> navigateTo(PageId::Home)
       │
       └─> Home page affiché !
```

### 2. Données capteur (ESP32 → Pi)

```
loop()
  │
  ├─> BME280.read() → tC, hPct, pPa
  │
  ├─> Model.setTempInX10(tC * 10)
  │    └─> dirty |= DK_HM_TempIn
  │
  ├─> proto.sendWithAck("SENS", ...)
  │    └─> Pi répond "ACK 2 OK"
  │         └─> touchHeartbeat()
  │
  └─> UIManager.update()
       └─> renderPartials()
            └─> TempWidget refresh partiel
```

### 3. Données météo (Pi → ESP32)

```
Pi envoie: "5 WEATHER code=2 tOut=245 hum=65"
  │
  └─> onMsg() callback
       │
       ├─> Model.setWeather(2)
       │    └─> dirty |= DK_HM_Weather
       │
       ├─> Model.setTempOutX10(245)
       │    └─> dirty |= DK_HM_TempOut
       │
       └─> UIManager.update()
            └─> WeatherWidget refresh partiel
```

## Optimisations e-paper

### 1. Full vs Partial refresh

**Full refresh** (lent, ~2 secondes) :
- Changement de page
- Transitions importantes
- Reset UI

**Partial refresh** (rapide, ~300ms) :
- Update de widget (temp, horloge, etc.)
- Budget : max 2 par update (éviter ghosting)

### 2. Dirty tracking

**Principe** :
- Model marque des flags dirty quand les données changent
- UI consomme les flags et marque les widgets dirty
- Seulement les widgets dirty sont refresh

**Flags** (bitmask) :
```cpp
DK_OB_Active    = 1 << 0,  // Onboarding actif
DK_LD_Active    = 1 << 1,  // Loading actif
DK_HM_WifiBle   = 1 << 2,  // WiFi/BLE status
DK_HM_TempIn    = 1 << 3,  // Température intérieure
DK_HM_TempOut   = 1 << 4,  // Température extérieure
DK_HM_Weather   = 1 << 5,  // Météo
DK_HM_Humidity  = 1 << 6,  // Humidité
DK_HM_Clock     = 1 << 7,  // Horloge
```

### 3. Budget system

```cpp
renderPartials(display, model, budget=2)
  │
  ├─> if (statusIcons.dirty && budget) → refresh, budget--
  ├─> if (clock.dirty && budget) → refresh, budget--
  ├─> if (weather.dirty && budget) → refresh, budget-- (skip si budget=0)
  └─> if (tempIn.dirty && budget) → refresh, budget-- (skip si budget=0)
```

**Ordre de priorité** : status → clock → weather → temp

## Main loop simplifié

```cpp
void loop() {
  // 1) Communication
  proto.poll();              // UART Pi ↔ ESP32
  powerBtn.update();         // Heartbeat + bouton
  
  // 2) Sensors
  bme.read(tC, pPa, hPct);   // BME280
  model.setTempInX10(...);   // Update model
  fan.update(tC);            // Contrôle ventilateur
  
  // 3) Send data to Pi
  if (time_to_send) {
    proto.sendWithAck("SENS", ...);
  }
  
  // 4) UI (automatic)
  uiManager.update();        // Gère tout !
  
  delay(100);
}
```

**Total** : ~20 lignes dans loop() vs ~200 lignes sans UIManager !
