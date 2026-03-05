# Sending HA Status to the ESP32

This document analyses how to fetch Home Assistant and system status on the Pi (App) and send it to the ESP32 for display. It covers: **HA Core status**, **Supervisor status**, **Internet connectivity** (LAN/WiFi), and **exposure on the internet** (HA Cloud, DuckDNS, Cloudflare tunnel, etc.).

---

## 1. Target Statuses (What to Show on the ESP32)

| Status | Meaning | Source of truth |
|--------|--------|------------------|
| **HA Core** | Core is running and API responds | Supervisor or Core API |
| **Supervisor** | Supervisor is healthy | Supervisor API |
| **Internet** | Host has connectivity (LAN and/or WiFi) | Supervisor network/host APIs |
| **Exposed** | Instance reachable from internet (Cloud, DuckDNS, tunnel, etc.) | Core config / integrations / Supervisor add-ons |

The ESP32 already has a **status icons** row (WiFi, BLE, Zigbee, Thread, IR, 433MHz, Matter). Today only **WiFi** is driven by the Pi (set to true on READY, false on HALTED). The others can be repurposed or extended for the new statuses (e.g. WiFi = internet, one icon = Core, one = Supervisor, one = Exposed), or we add a dedicated “HA status” payload.

---

## 2. Where to Get the Information

### 2.1 Supervisor API (from the App container)

The App runs in a container and talks to the Supervisor with `SUPERVISOR_TOKEN` and `SUPERVISOR_URL` (or `http://172.30.32.2`). Base path: `{SUPERVISOR_URL}/` (no `/core/api` prefix for Supervisor endpoints).

| Endpoint | Purpose | Useful fields |
|----------|---------|----------------|
| **GET /supervisor/info** | Supervisor version and state | `result` = "ok" + version; if request fails or returns error → Supervisor unhealthy |
| **GET /core/info** | Core (Home Assistant) info | Version, state; if 401/5xx or unreachable → Core down or not ready |
| **GET /core/api** | Simple Core reachability (already used in ha_api) | 200 = Core API up |
| **GET /host/info** | Host (HAOS) info | Host name, OS, features; optional for “system” display |
| **GET /network/info** or **GET /network/list** | Network interfaces | Interfaces with `type` (ethernet / wireless), `connected`, `primary` → derive “LAN” vs “WiFi” and “has internet” |

References:

- [Supervisor API (Endpoints/Models)](https://developers.home-assistant.io/docs/api/supervisor/) – official docs; some links may be under “add-ons” or “apps”.
- Supervisor source: [home-assistant/supervisor](https://github.com/home-assistant/supervisor) (REST handlers under `supervisor/api/`).

**Derived logic:**

- **Core OK**: Same as today: GET `{base}/core/api` returns 200 (we already have `_check_core_available()`).
- **Supervisor OK**: GET `{base}/supervisor/info` returns 200 and body indicates success.
- **Internet (LAN/WiFi)**: From `GET /network/info` (or equivalent): at least one interface with `connected == true`; optionally distinguish `type == "ethernet"` (LAN) vs `type == "wireless"` (WiFi).

### 2.2 Home Assistant Core API (via Supervisor proxy)

Core is reached via Supervisor proxy at `{base}/core/api/...`.

| Endpoint | Purpose | Useful for |
|----------|---------|------------|
| **GET /core/api/config** | Core config | Version, location; not needed for “exposed” |
| **GET /core/api/states** | States | Query specific entities (e.g. cloud/remote_ui or add-on states) |

**“Exposed on internet”:**

- **Nabu Casa (HA Cloud)**: Often exposed as an integration; state may be in an entity or in config. No single standard REST endpoint; sometimes derived from `cloud` component or a “cloud” related entity.
- **DuckDNS / Let’s Encrypt / Cloudflare tunnel**: Usually run as **Supervisor add-ons**. Their “state” (running, failed) can be obtained from the **Supervisor add-ons list** (e.g. `GET /addons` or `/store/addons` and filter by slug, then check state). If an add-on is “started” and its slug is one of `a0d7b954_duckdns`, `core_duckdns`, `cloudflare`, etc., we can consider “exposed” as true (optionally with which method).
- **Alternative**: Expose a small config option in our App (e.g. “exposed” = cloud | duckdns | tunnel | none) and let the user set it, or try to infer from add-ons list.

So:

- **Core status**: Supervisor + Core API (already in place).
- **Supervisor status**: Supervisor `/supervisor/info`.
- **Internet (LAN/WiFi)**: Supervisor `/network/info` (or list interfaces).
- **Exposed**: Prefer Supervisor add-ons list; optionally Core entities if we standardise on specific entity_ids for “remote access” or “cloud”.

---

## 3. Current State in the Project

- **Pi → ESP32**: Today we send only **READY** (no payload) and **STATUS** (no payload). READY means “Pi is up”; STATUS is a heartbeat. The ESP32 sets `model.setWifi(true)` on READY and `setWifi(false)` on HALTED.
- **ESP32**: `HomeState` has booleans: `wifi_ok`, `ble_ok`, `zigbee_ok`, `thread_ok`, `ir_ok`, `mhz433_ok`, `matter_ok`. Only `wifi_ok` is updated from the Pi; the rest are unused or set by serial commands for tests.
- **Protocol**: ASCII protocol with verb + optional key-value pairs (e.g. `WEATHER code=2 tOut=245`, `CLOCK hh=14 mm=32`, `LANG id=0`). We can add a **STATUS** (or **HA_STATUS**) message with KVs for core, supervisor, internet, exposed.

---

## 4. Proposed Integration

### 4.1 Fetching on the App (Python)

- **New module or extend `ha_api.py`**:
  - **Supervisor-only** (no Core proxy):
    - `get_supervisor_info()` → GET `{base}/supervisor/info` → return dict or None; “Supervisor OK” = 200 and `result == "ok"` (or equivalent).
    - `get_network_info()` → GET `{base}/network/info` (or the list endpoint used by Supervisor) → return list of interfaces; derive “has_internet”, “lan_connected”, “wifi_connected”.
  - **Core** (existing + optional):
    - Keep `_check_core_available()` for “Core OK”.
  - **Exposed** (optional for v1):
    - `get_addons_list()` → GET `{base}/addons` or similar → filter by slugs (e.g. DuckDNS, Cloudflare); “exposed” = at least one such add-on is “started”. If the endpoint is not available or restricted, fall back to config option or skip for first version.

- **Caching / rate limiting**: These calls should be done at a fixed interval (e.g. every 30–60 s), not on every loop. Reuse the same pattern as weather/clock (e.g. a small “status” service or a method called from the main loop with a timer).

### 4.2 Sending to the ESP32

**Chosen approach: extend STATUS with KV.** Send a single **STATUS** message with key-value pairs, e.g.  
`STATUS core=1 sup=1 net=1 wifi=1 lan=1 ext=1`  
(1 = ok, 0 = not ok). The ESP32 already accepts STATUS; we add parsing of KVs in `onMsg` and update the model. No new verb; STATUS becomes both “heartbeat” and “status bundle”. If no KVs are sent, ESP32 keeps current behaviour (e.g. READY still sets wifi_ok; optional: treat STATUS with no KVs as “alive” only and leave icons unchanged).

### 4.3 ESP32 Side

- **Protocol**: Keep **STATUS** in `authorizeIncoming`. In `onMsg`, when verb is STATUS and `msg.kvCount > 0`, parse KVs, e.g.:
  - `core` (0/1), `sup` (0/1), `net` (0/1), `lan` (0/1), `wifi` (0/1), `ext` (0/1).
  - `zigbee` (0/1), `thread` (0/1), `matter` (0/1) → Zigbee/Thread/Matter status (icons in status bar).
- **Model**: Reuse existing booleans; Zigbee/Thread/Matter are driven by the Pi from add-ons and Core integrations (see 4.5).

### 4.5 Zigbee, Thread, Matter (incl. multipan)

Detection works in all common setups:

- **Zigbee**
  - **ZHA** (Zigbee Home Automation): Core integration only (no add-on). Detected via Core entities `zha.*`.
  - **Zigbee2MQTT**: Add-on whose slug contains `zigbee2mqtt` (any repo), or Core entities `zigbee2mqtt.*`.
- **Thread**
  - **OpenThread Border Router (OTBR)**: Add-on whose slug contains `openthread` or `otbr` (e.g. `core_openthread_border_router`), or Core entities `otbr.*`.
  - **Silabs Multiprotocol**: Add-on whose slug contains `silabs` (multipan Zigbee+Thread on one stick).
  - Any other Thread add-on: slug contains `thread`.
- **Matter**: Add-on whose slug contains `matter`, or Core entities `matter.*`.

With a **multipan** stick (e.g. Silabs Multiprotocol), Zigbee and Thread can both be active: Zigbee via Zigbee2MQTT or ZHA, Thread via the same add-on or OTBR. Each protocol is detected independently (add-on list + Core entity list). No user config required; slugs are matched by substring so any repo (core_*, community repo id_*) is supported.

### 4.4 Summary of Steps

1. **App – Supervisor API**: Implement `get_supervisor_info()`, `get_network_info()`, and add-ons list (see section 7) in `ha_api.py` or a dedicated module; timeouts and error handling; no Core dependency for Supervisor/network/addons.
2. **App – Status aggregation**: A **StatusHandler** (see section 5) that at a fixed interval builds core_ok, supervisor_ok, internet_ok, lan_ok, wifi_ok, exposed_ok and sends **STATUS** with KVs via `esp32_comm.send_command("STATUS", [KV("core", "1"), ...])`.
3. **Connection manager**: Either (a) keep sending a lightweight STATUS (no KVs) as heartbeat from connection_manager and have StatusHandler send a separate “rich” STATUS with KVs on its own timer, or (b) have the single periodic message sent by StatusHandler (STATUS with KVs) and remove the bare STATUS from connection_manager so one message does both. Option (b) is simpler: one STATUS with KVs every 30 s = heartbeat + status bundle.
4. **ESP32**: In `onMsg` for STATUS, parse KVs and update model + dirty flags; wire icons to booleans.
5. **Docs**: Update `ESP32_RASP_COM.md` and protocol docs with STATUS KV format.

---

## 5. Code Architecture (Event-Driven, Flexible)

### 5.1 Current Pattern (Weather, Clock)

- **Handlers** own their **interval** and **threading.Timer**; they do not run in the main loop.
- **Initialization**: When connection is established, `force_update()` is called once, then `start_periodic_updates()`.
- **Main loop**: Only checks connection and sensor updates; **no** direct weather/clock logic. Timers fire in background and call `_send_weather()` / `_send_clock()` if `_is_connected_callback()` is True.
- **Lifecycle**: `stop_periodic_updates()` in `finally` on shutdown.

### 5.2 Status: Same Pattern (StatusHandler)

- **New handler**: `handlers/status_handler.py` – **StatusHandler** with:
  - `update_interval` (e.g. 30 s, configurable).
  - `esp32_comm`, `ha_api` (or a dedicated supervisor/client that exposes `get_supervisor_info`, `get_network_info`, `get_addons_list`, and Core check).
  - `force_update()`, `start_periodic_updates()`, `stop_periodic_updates()`, `set_connection_check()`, same as Weather/Clock.
  - On tick: call API to build status dict (core, sup, net, wifi, lan, ext), then `esp32_comm.send_command("STATUS", list_of_kv)`.
- **Single periodic message**: The STATUS-with-KV message is the only periodic “heartbeat” when StatusHandler is used; connection_manager can **stop** sending its own bare STATUS and instead rely on StatusHandler to send STATUS with KVs at the same interval (e.g. 30 s). Connection state (CONNECTED, etc.) still comes from `connection_manager.update()` and `last_message_time`; the “message we send” is just produced by StatusHandler.
- **Alternative (two messages)**: Keep connection_manager sending a simple STATUS every 30 s for heartbeat, and StatusHandler sending a separate STATUS with KVs every 30 s (or 60 s). That duplicates traffic; one STATUS with KVs is enough.

### 5.3 Where Supervisor/Core API Lives

- **Option A – Extend `api/ha_api.py`**: Add `get_supervisor_info()`, `get_network_info()`, `get_addons_list()` to `HomeAssistantAPI`. Keeps one client and one place for `base_url` + token. Supervisor endpoints use the same `base_url` (Supervisor), Core uses `base_url + "/core/api"`.
- **Option B – New `api/supervisor_api.py`**: Dedicated client for Supervisor-only endpoints (info, network, addons). Clear separation; `StatusHandler` would take both `ha_api` (for Core check) and `supervisor_api` (for supervisor/network/addons), or a single “status client” that wraps both.

Recommendation: **Option A** (extend `ha_api.py`) for now; if the file grows too large, split later into `supervisor_api.py` and keep Core in `ha_api.py`.

### 5.4 Main Loop Integration

- **Initialization loop**: After connection, call `status_handler.force_update()`, then `status_handler.start_periodic_updates()` (same as weather/clock).
- **Main loop**: No status logic; only connection check and sensor updates. Status runs on a timer.
- **Shutdown**: `status_handler.stop_periodic_updates()` in `finally`.
- **Connection manager**: Remove the “send STATUS” branch from `send_periodic_message()` when state is CONNECTED, so only StatusHandler sends STATUS (with KVs). CONNECTING/RECONNECTING/DISCONNECTED still send READY from connection_manager.

---

## 6. WebSockets (Core vs Supervisor)

### 6.1 Core WebSocket (Available)

- **URL**: `ws://supervisor/core/websocket` (or the same host as `SUPERVISOR_URL` with scheme `ws://` and path `/core/websocket`). The Supervisor proxies WebSocket to Core.
- **Auth**: Same token as REST: `SUPERVISOR_TOKEN` (e.g. as password in the WebSocket handshake or via query param, per HA docs).
- **Use case**: **Real-time state/event stream** from Home Assistant (entity state changes, events). Useful if we want the ESP32 to react immediately to HA events (e.g. “light turned off”, “alarm”) without polling. For **status** (Core up, Supervisor up, network, exposed), polling REST every 30 s is enough and keeps the App simple; no WebSocket required for status.
- **When to consider WS**: Later, if we add “live” entity or event-driven features (e.g. push a notification to the display when a door opens). Implementation: one background thread or async task that holds the WebSocket, parses messages, and either updates an in-memory state or triggers a send to the ESP32. Adds complexity (reconnect, backoff, message types).

**Conclusion for status**: Stay with **REST + timer** for now. WebSocket is interesting for future event-driven features, not for the current status bundle.

### 6.2 Supervisor WebSocket

- The **Supervisor** does **not** expose a public WebSocket API for add-ons. All Supervisor data (info, network, addons) is obtained via **REST**. So we only poll Supervisor endpoints at our chosen interval.

---

## 7. Other Apps (Add-ons): Detection and Access

### 7.1 How to List Other Apps

- **Supervisor API**: The Supervisor exposes REST endpoints to list add-ons. The exact path may be one of:
  - `GET /addons` – list of installed add-ons (to be confirmed in Supervisor source or [python-supervisor-client](https://github.com/home-assistant-libs/python-supervisor-client)).
  - `GET /store/addons` – store listing (all available); we need **installed** add-ons and their **state**.
- **Self-info**: `GET /addons/self/info` is documented for the current add-on; it returns our own slug, version, state, etc.
- **Discovery**: In the Supervisor GitHub repo, look under `supervisor/api/` for routes like `addons`, `addons/list`, or `store`. The [Addon model](https://developers.home-assistant.io/docs/api/supervisor/models) includes: **name**, **slug**, **version**, **version_latest**, **state** (e.g. "started", "stopped"), **installed**, **available**, and optionally **description**, **icon**, **update_available**, **system_managed**.

### 7.2 What We Need for “Exposed” and General Use

- **Exposed on internet**: Filter the add-ons list by **slug** (e.g. `a0d7b954_duckdns`, `core_duckdns`, `a0d7b954_cloudflare`, or slugs for Nabu Casa / Cloudflare tunnel add-ons). If any of these has **state == "started"**, set `ext=1` in STATUS. Slugs can be configured in our App (e.g. a list of “exposed” add-on slugs) so we do not hardcode.
- **Other useful info** (optional for display or logic):
  - **Update available**: `update_available` per add-on; we could show a “updates available” icon on the ESP32 if any critical add-on has an update.
  - **State of specific Apps**: e.g. Zigbee2MQTT, Mosquitto – if we want to drive Zigbee/Matter icons from actual add-on state, we can map slug → icon (zigbee_ok = True if addon slug X is started). That requires a small mapping (slug → our model flag) in config or code.

### 7.3 Access Control (403 on GET /addons)

- The Supervisor may return **403 Forbidden** for `GET /addons` depending on the add-on's API role. With the default role (`hassio_api: true`), listing all add-ons is often not allowed.
- **Current behaviour**: The app treats 403 on `/addons` as "add-ons list unavailable": it logs a **WARNING** (not ERROR), returns no list, and status shows **ext=0**. Core, Supervisor, network and LAN/WiFi status are unchanged.
- **If you need ext=1 (exposed)**: Ensure the add-on has Supervisor API access; check Supervisor/docs for the role required to list add-ons. Alternatively, rely on user config or `GET /addons/self/info` only.

---

## 8. References

- [Developing an app](https://developers.home-assistant.io/docs/apps/) (Apps documentation)
- [Supervisor API](https://developers.home-assistant.io/docs/api/supervisor/) (endpoints/models)
- [Home Assistant Supervisor (GitHub)](https://github.com/home-assistant/supervisor) for exact endpoint paths and response shapes
- Existing project: `ha-box/rootfs/usr/bin/ha-box/api/ha_api.py`, `communication/connection_manager.py`, `esp/ESPHOMEASSISTANT/ESPHOMEASSISTANT.ino` (onMsg, authorizeIncoming), `esp/ESPHOMEASSISTANT/model/state_home.h`, `ui/widgets/widget_status_icons.h`
