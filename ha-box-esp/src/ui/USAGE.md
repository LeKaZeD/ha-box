# UIManager — Page lifecycle

## Architecture

```
UIManager
  ├─ Model (shared state)
  ├─ Display (e-paper)
  └─ Pages
      ├─ PageOnboarding (QR code + link)
      ├─ PageLoading (reason + animation)
      ├─ PageHome (dashboard)
      ├─ PageSettings (brightness / sound / language)
      └─ PageShutdown (power-off screen)
```

## Page lifecycle

Navigation calls `onExit()` on the current page, switches to the new page, calls `onEnter()`, then `drawFull()` for a full e-paper refresh.

Each `update()` call runs `tick()` (animations, timers), then — if the model has dirty flags — `applyDirty()` to mark affected widgets followed by `renderPartials()` with a budget of 2 partial refreshes.

## Model → UI flow

UART messages are received by `proto.poll()`, dispatched to handlers in `main.cpp`, which call model setters (e.g. `model.setWeather()`, `model.setClock(hh, mm, ss, receivedAtMs)`). Each setter marks a dirty flag. On the next `uiManager.update()`, dirty widgets render via fast partial refresh.

Humidity is NOT received via UART. It comes from the BME280 sensor read directly by the ESP32 every 5 s and is set in the model locally.

## Key properties

- **Simple main loop**: `proto.poll()` → sensors → `uiManager.update()`
- **Clear navigation**: `navigateTo(PageId::Home|Settings|Loading|…)`
- **Lifecycle hooks**: `onEnter()`, `onExit()`, `tick()`
- **Automatic dirty tracking**: optimised for e-paper — no unnecessary refreshes
- **Partial refresh budget**: max 2 partial refreshes per update cycle to prevent ghosting
- **Model/View separation**: UART handler → model setter → dirty flag → widget refresh
