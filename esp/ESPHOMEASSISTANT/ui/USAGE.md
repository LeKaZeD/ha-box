# UIManager - Gestion du cycle de vie des pages

## Architecture

```
UIManager
  ├─ Model (données partagées)
  ├─ Display (e-paper UC8253)
  └─ Pages
      ├─ PageOnboarding (QR code + lien)
      ├─ PageLoading (raison + animation)
      └─ PageHome (dashboard)
```

## Cycle de vie d'une page

```
navigateTo(PageId::Home)
  │
  ├─> currentPage->onExit(model)    // Cleanup de la page précédente
  │
  ├─> currentPage = &pageHome_      // Switch
  │
  ├─> currentPage->onEnter(model)   // Init de la nouvelle page
  │
  └─> currentPage->drawFull(...)    // Render complet (full refresh e-paper)

Ensuite, chaque update():
  │
  ├─> currentPage->tick(model)                 // Logique locale (animations, timers, etc.)
  │
  ├─> if (model.hasDirty())                    // Si le modèle a changé
  │     ├─> applyDirty(dirtyMask)             // Marquer widgets comme dirty
  │     └─> renderPartials(display, model, 2) // Partial refresh (rapide)
```

## Usage dans le main

### 1. Setup

```cpp
#include "ui/ui_manager.h"
#include "epd_uc8253.h"
#include "canvas_1bpp.h"

// Display
EPD_UC8253 epd;
Canvas1bpp canvas(240, 416);

// Model
Model model;

// UIManager
UIManager<Canvas1bpp> uiManager(canvas, model);

void setup() {
  // Init hardware
  epd.begin();
  canvas.begin(&epd);
  
  // Config fonts (optional)
  uiManager.pageHome().setFonts(&Roboto20, &Roboto24, &Roboto16);
  uiManager.pageLoading().setFonts(&Roboto16);
  
  // Start on Loading page
  uiManager.begin(PageId::Loading);
  model.setLoadingReason("Connecting...");
}
```

### 2. Loop simplifié

```cpp
void loop() {
  // 1) Poll UART (met à jour le model via callbacks)
  proto.poll();
  
  // 2) Update UI (gère automatiquement le render)
  uiManager.update();
  
  // 3) Logique métier (navigation, etc.)
  if (some_condition) {
    uiManager.navigateTo(PageId::Home);
  }
  
  delay(100);
}
```

### 3. Mise à jour du model depuis UART

```cpp
static void onMsg(const AsciiProto::Message& msg, void* user) {
  Model* model = (Model*)user;
  
  if (strcmp(msg.verb, "WEATHER") == 0) {
    uint8_t code = atoi(msg.get("code"));
    int16_t tOut = atoi(msg.get("tOut"));
    uint8_t hum = atoi(msg.get("hum"));
    
    model->setWeather(code);
    model->setTempOutX10(tOut);
    model->setHumidity(hum);
    // dirty flags automatiquement set
  }
  
  if (strcmp(msg.verb, "CLOCK") == 0) {
    model->setClock(atoi(msg.get("hh")), atoi(msg.get("mm")));
  }
}

void setup() {
  // ...
  proto.setMessageCallback(onMsg, &model);
}
```

## Navigation automatique

```cpp
void loop() {
  proto.poll();
  
  // Navigation conditionnelle
  switch (uiManager.getCurrentPageId()) {
    case PageId::Loading:
      // Si connexion établie, aller à Home
      if (model.home.wifi_ok) {
        uiManager.navigateTo(PageId::Home);
      }
      break;
      
    case PageId::Onboarding:
      // Si onboarding terminé, aller à Loading
      if (!model.onboarding.active) {
        model.setLoadingReason("Connecting Wi-Fi...");
        uiManager.navigateTo(PageId::Loading);
      }
      break;
  }
  
  uiManager.update();
  delay(100);
}
```

## Avantages

✅ **Main loop simple** : Juste `uiManager.update()`
✅ **Navigation claire** : `navigateTo(PageId::Home)`
✅ **Lifecycle hooks** : `onEnter()`, `onExit()`, `tick()`
✅ **Dirty tracking automatique** : Optimisé pour e-paper
✅ **Partial refresh** : Budget system pour économiser l'énergie
✅ **Séparation Model/View** : UART → Model → UI

## Exemple complet

```cpp
Model model;
UIManager<Canvas1bpp> uiManager(canvas, model);

void setup() {
  // Hardware
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  epd.begin();
  canvas.begin(&epd);
  
  // UART callbacks
  proto.begin();
  proto.setMessageCallback(onMsg, &model);
  proto.setAckCallback(onAck, nullptr);
  
  // UI
  uiManager.begin(PageId::Onboarding);
  model.setOnboardingActive(true);
  model.setOnboardingQrPayload("WIFI:T:WPA;S:MySSID;P:password;;");
}

void loop() {
  // 1) Communication
  proto.poll();
  powerBtn.update();
  
  // 2) Sensors (BME280)
  float tC, pPa, hPct;
  if (bme.read(tC, pPa, hPct)) {
    model.setTempInX10((int16_t)(tC * 10.0f));
    model.setHumidity((uint8_t)hPct);
  }
  
  // 3) Navigation (state machine)
  handleNavigation();
  
  // 4) UI update (automatic rendering)
  uiManager.update();
  
  delay(100);
}

void handleNavigation() {
  switch (uiManager.getCurrentPageId()) {
    case PageId::Onboarding:
      if (wifiConnected) {
        model.setOnboardingActive(false);
        model.setLoadingReason("Syncing time...");
        uiManager.navigateTo(PageId::Loading);
      }
      break;
      
    case PageId::Loading:
      if (allReady) {
        uiManager.navigateTo(PageId::Home);
      }
      break;
  }
}
```
