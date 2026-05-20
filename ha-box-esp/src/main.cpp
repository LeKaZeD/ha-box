#include <Arduino.h>
#include <Wire.h>
#include "driver/rtc_io.h"
#include "config.h"
#include "settings_persistence.h"
#include "buzzer.h"
#include "BME280.h"
#include "TMP102.h"
#include "PowerButton.h"
#include "FanController.h"


#include <GxEPD2_BW.h>
#include "gdey/GxEPD2_370_GDEY037T03.h"
#include "touch_ft6336.h"
#include "model/model.h"
#include "ui/ui_manager.h"

//*******************************************
GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(PIN_CS, PIN_DC, PIN_RST, PIN_BUSY));
TouchFT6336 touch(0x38);

Model model;
UIManager<GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT>> uiManager(display, model);

#include "ui/ui_entry.h"

//*******************************************
static uint32_t lastSensMs = 0;
static uint32_t lastBmeMs = 0;
static float lastTc = 0.0f, lastPPa = 0.0f, lastHPct = 0.0f;

// petits buffers pour convertir float -> string
static char tBuf[16];
static char hBuf[16];
static char pBuf[16];
static char boxTcBuf[16];

AsciiProto proto(Serial);

BME280Min bme(0x76);
TMP102    tmp102(TMP102_ADDR);
static bool s_tmp102Ok = false;
static bool s_bmeOk    = false;

PowerButton::Config pbCfg{
  .btnPin = BTN_PIN,
  .j2Pin  = J2_PULSE_PIN,
  .j2ActiveHigh = true,
  .j2PulseMs = 150,
  .j2ResetMs = 5500,
  .debounceMs = 30,
  .minClickMs = 120,
  .maxClickMs = 2000,
  .longPressMs = 5000,
  .hbTimeoutMs = 300000,
  .shutdownPendingTimeoutMs = 60000,
  .haltingTimeoutMs = 15000,
  .piAlivePin = PIN_PI_ALIVE,
  .token = "changeme"
};

PowerButton powerBtn(pbCfg, proto);

FanController::Config fanCfg{
  .pwmPin = PWM_PIN,
  .pwmFreqHz = PWM_FREQ_HZ,
  .pwmResBits = PWM_RES_BITS,
  .tFanOff = 25.0f,
  .tFanOn  = 28.0f,
  .tFull   = 35.0f,
  .dutyMin = 10,
  .dutyMax = 255,
  .safeDuty = 255
};

FanController fan(fanCfg);

static bool authorizeIncoming(const AsciiProto::Message& msg, void* user, const char** err) {
  // Allowlist of commands accepted from the Pi
  if (strcmp(msg.verb, "READY") == 0) return true;
  if (strcmp(msg.verb, "HALTED") == 0) return true;
  if (strcmp(msg.verb, "SHUTDOWN_ACCEPTED") == 0) return true;
  if (strcmp(msg.verb, "STATUS") == 0) return true;
  if (strcmp(msg.verb, "WEATHER") == 0) return true;
  if (strcmp(msg.verb, "CLOCK") == 0) return true;
  if (strcmp(msg.verb, "LANG") == 0) return true;
  if (strcmp(msg.verb, "FAN") == 0) return true;
  if (strcmp(msg.verb, "VERSION") == 0) return true;
  if (strcmp(msg.verb, "OTA") == 0) return true;
  if (strcmp(msg.verb, "DAYMODE") == 0) return true;
  *err = "unknown_cmd";
  return false;
}

// ---- Per-verb message handlers ----

static void handleReady(const AsciiProto::Message& msg) {
  (void)msg;
  powerBtn.setPiState(PowerButton::PiState::ON);
  setLoadingProfileDefault(uiManager);
  model.setLoadingReason(i18n::loading().reasonDefault);
  const PageId cur = uiManager.getCurrentPageId();
  if (cur == PageId::Loading || cur == PageId::Shutdown) {
    uiManager.navigateTo(PageId::Home);
  }
}

static void handleHalted(const AsciiProto::Message& msg) {
  (void)msg;
  powerBtn.setPiState(PowerButton::PiState::OFF);
  model.setNet(false);
  model.setLan(false);
  model.setWifi(false);
  model.setExt(false);
  fan.setEnabled(false);
  uiManager.navigateTo(PageId::Shutdown);
}

static void handleWeather(const AsciiProto::Message& msg) {
  for (uint8_t i = 0; i < msg.kvCount; i++) {
    if (strcmp(msg.kv[i].key, "code") == 0) {
      model.setWeather((uint8_t)constrain(atoi(msg.kv[i].val), 0, WEATHER_CODE_MAX));
    } else if (strcmp(msg.kv[i].key, "tOut") == 0) {
      model.setTempOutX10((int16_t)atoi(msg.kv[i].val));
    }
  }
}

static void handleClock(const AsciiProto::Message& msg) {
  uint8_t hh = 0, mm = 0, ss = 0;
  for (uint8_t i = 0; i < msg.kvCount; i++) {
    if (strcmp(msg.kv[i].key, "hh") == 0) { int v = atoi(msg.kv[i].val); hh = (v >= 0 && v <= 23) ? (uint8_t)v : 0; }
    if (strcmp(msg.kv[i].key, "mm") == 0) { int v = atoi(msg.kv[i].val); mm = (v >= 0 && v <= 59) ? (uint8_t)v : 0; }
    if (strcmp(msg.kv[i].key, "ss") == 0) { int v = atoi(msg.kv[i].val); ss = (v >= 0 && v <= 59) ? (uint8_t)v : 0; }
  }
  model.setClock(hh, mm, ss, millis());
}

static void handleLang(const AsciiProto::Message& msg) {
  for (uint8_t i = 0; i < msg.kvCount; i++) {
    if (strcmp(msg.kv[i].key, "id") == 0) {
      uint8_t langId = (uint8_t)constrain(atoi(msg.kv[i].val), 0, 1);
      if (langId == model.settings.language) return;
      model.setLanguage(langId);
      saveSettingsIfChanged(model);
      rebuildPageAfterLanguageChange(langId, uiManager);
      Serial2.printf("[cmd] LANG set to %s (id=%u)\n", langId == 0 ? "FR" : "EN", (unsigned)langId);
      break;
    }
  }
}

static void handleFan(const AsciiProto::Message& msg) {
  bool hasEn = false, hasTOn = false, hasTFull = false;
  bool en = false;
  float tOn = 0.0f, tFull = 0.0f;
  for (uint8_t i = 0; i < msg.kvCount; i++) {
    if (strcmp(msg.kv[i].key, "en") == 0) {
      en = atoi(msg.kv[i].val) != 0; hasEn = true;
    } else if (strcmp(msg.kv[i].key, "tOn") == 0) {
      tOn = atof(msg.kv[i].val); hasTOn = true;
    } else if (strcmp(msg.kv[i].key, "tFull") == 0) {
      tFull = atof(msg.kv[i].val); hasTFull = true;
    }
  }
  if (hasEn) fan.setEnabled(en);
  if (hasTOn && hasTFull && tFull > tOn) fan.setCurve(tOn, tFull);
  Serial2.printf("[FAN] en=%d tOn=%.1f tFull=%.1f\n", (int)en, tOn, tFull);
  // Persist only when we have a complete config (all three keys present).
  if (hasEn && hasTOn && hasTFull && tFull > tOn) {
    saveFanConfig({ .enabled = en, .tOn = tOn, .tFull = tFull });
  }
}

static void handleVersion(const AsciiProto::Message& msg) {
  (void)msg;
  AsciiProto::KV verKv[] = { {"ver", ""} };
  strncpy(verKv[0].val, FIRMWARE_VERSION, sizeof(verKv[0].val) - 1);
  proto.sendCommand("VERSION", verKv, 1);
}

static void handleOta(const AsciiProto::Message& msg) {
  (void)msg;
  setLoadingProfileFirmwareUpdate(uiManager);
  model.setLoadingAnimFrame(0);
  model.setLoadingReason(i18n::loading().reasonFirmwareUpdate);
  uiManager.navigateTo(PageId::Loading);
}

static void handleDaymode(const AsciiProto::Message& msg) {
  for (uint8_t i = 0; i < msg.kvCount; i++) {
    if (strcmp(msg.kv[i].key, "mode") == 0) {
      uint8_t m = (uint8_t)constrain(atoi(msg.kv[i].val), 0, 1);
      model.setDayMode(m);
      Serial2.printf("[cmd] DAYMODE set to %s\n", m == 0 ? "day" : "night");
      break;
    }
  }
}

// STATUS: optional KV (core, sup, net, lan, wifi, ext, zigbee, thread, matter). If no KV, heartbeat only.
static void handleStatus(const AsciiProto::Message& msg) {
  for (uint8_t i = 0; i < msg.kvCount; i++) {
    const bool ok = (atoi(msg.kv[i].val) != 0);
    if      (strcmp(msg.kv[i].key, "core")   == 0) model.setCore(ok);
    else if (strcmp(msg.kv[i].key, "sup")    == 0) model.setSup(ok);
    else if (strcmp(msg.kv[i].key, "net")    == 0) model.setNet(ok);
    else if (strcmp(msg.kv[i].key, "lan")    == 0) model.setLan(ok);
    else if (strcmp(msg.kv[i].key, "wifi")   == 0) model.setWifi(ok);
    else if (strcmp(msg.kv[i].key, "ext")    == 0) model.setExt(ok);
    else if (strcmp(msg.kv[i].key, "zigbee") == 0) model.setZigbee(ok);
    else if (strcmp(msg.kv[i].key, "thread") == 0) model.setThread(ok);
    else if (strcmp(msg.kv[i].key, "matter") == 0) model.setMatter(ok);
    else if (strcmp(msg.kv[i].key, "mhz433") == 0) model.setMhz433(ok);
    else if (strcmp(msg.kv[i].key, "warn")   == 0) model.setWarnings((uint8_t)atoi(msg.kv[i].val));
    else if (strcmp(msg.kv[i].key, "err")    == 0) model.setErrors((uint8_t)atoi(msg.kv[i].val));
  }
}

// ---- Dispatcher ----

static void onMsg(const AsciiProto::Message& msg, void* user) {
  (void)user;
  Serial2.printf("[RX] id=%lu verb=%s\n", (unsigned long)msg.id, msg.verb);
  for (uint8_t i = 0; i < msg.kvCount; i++)
    Serial2.printf("  %s=%s\n", msg.kv[i].key, msg.kv[i].val);
  powerBtn.touchHeartbeat();

  if (strcmp(msg.verb, "READY")   == 0) return handleReady(msg);
  if (strcmp(msg.verb, "HALTED")  == 0) return handleHalted(msg);
  if (strcmp(msg.verb, "WEATHER") == 0) return handleWeather(msg);
  if (strcmp(msg.verb, "CLOCK")   == 0) return handleClock(msg);
  if (strcmp(msg.verb, "LANG")    == 0) return handleLang(msg);
  if (strcmp(msg.verb, "FAN")     == 0) return handleFan(msg);
  if (strcmp(msg.verb, "VERSION") == 0) return handleVersion(msg);
  if (strcmp(msg.verb, "OTA")     == 0) return handleOta(msg);
  if (strcmp(msg.verb, "DAYMODE") == 0) return handleDaymode(msg);
  if (strcmp(msg.verb, "STATUS")  == 0) return handleStatus(msg);
}

static void onRawLine(const char* line, void* user) {
  Serial2.printf("[RX raw] %s\n", line);
  powerBtn.onUartLine(line);
}

static void onSettingsBackClick(void* user) {
  (void)user;
  saveSettingsIfChanged(model);
  setupUI_rebuildAndGoHome(uiManager, model);
}

static void onAck(const AsciiProto::Ack& ack, void* user) {
  Serial2.printf("[RX] ACK id=%lu %s %s\n",
              (unsigned long)ack.id,
              ack.ok ? "OK" : "ERR",
              ack.ok ? "" : ack.err);
  powerBtn.touchHeartbeat();
}

static void onPiStateChange(PowerButton::PiState st) {
  // Log simple
  if (st == PowerButton::PiState::ON) {
    Serial2.println("PiState=ON");
    model.setWifi(true);
    // Release the RTC hold set before deep sleep so LEDC can drive the pin again.
    rtc_gpio_hold_dis((gpio_num_t)PWM_PIN);
    fan.setEnabled(true);
  }
  else if (st == PowerButton::PiState::OFF) {
    Serial2.println("PiState=OFF");
    model.setNet(false);
    model.setLan(false);
    model.setWifi(false);
    model.setExt(false);

    // Stop fan and hold PWM_PIN LOW during deep sleep.
    // LEDC stops when the ESP enters deep sleep; without holding the pin it floats
    // and the MOSFET gate can rise above Vth, keeping the fan on.
    // GPIO 25 is RTC-capable so rtc_gpio_hold_en retains the LOW state during sleep.
    fan.setEnabled(false);
    ledcDetach(PWM_PIN);
    gpio_set_direction((gpio_num_t)PWM_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PWM_PIN, 0);
    rtc_gpio_hold_en((gpio_num_t)PWM_PIN);

    // Navigate to Shutdown from any page (condition was previously Home/Settings only,
    // which failed when the page was already Loading after SHUTDOWN_PENDING).
    if (uiManager.getCurrentPageId() != PageId::Shutdown) {
      uiManager.navigateTo(PageId::Shutdown);
    }
  }
  else if (st == PowerButton::PiState::SHUTDOWN_PENDING) {
    Serial2.println("PiState=SHUTDOWN_PENDING");
    // Show loading page immediately with shutdown reason so the screen updates
    // without waiting for HALTED (which can take up to shutdownPendingTimeoutMs).
    setLoadingProfileDefault(uiManager);
    model.setLoadingReason(i18n::loading().reasonShutdown);
    uiManager.navigateTo(PageId::Loading);
  }
  else {
    Serial2.println("PiState=HALTING");
    // Pi acknowledged shutdown — stay on Loading, wait for heartbeat silence.
    // No navigateTo here; transition to Shutdown happens when setPiState(OFF) fires.
  }
}



void setup() {

  // UART0: Pi communication (and bootloader flashing via esptool on the same lines).
  Serial.begin(115200, SERIAL_8N1);
  delay(200);

  // UART2: debug output only (GPIO16=RX, GPIO17=TX). Not connected to Pi.
  Serial2.begin(115200, SERIAL_8N1, DEBUG_RX, DEBUG_TX);

  proto.begin();
  proto.setRawLineCallback(onRawLine, nullptr);
  proto.setAuthorizeCallback(authorizeIncoming, nullptr);
  proto.setMessageCallback(onMsg, nullptr);
  proto.setAckCallback(onAck, nullptr);

  bme.beginWire(Wire, 21, 22, 100000);
  s_bmeOk = bme.begin();
  if (!s_bmeOk) {
    Serial2.println("BME280 init failed - continuing without ambient sensor");
  }

  tmp102.beginWire(Wire);
  s_tmp102Ok = tmp102.begin();
  if (!s_tmp102Ok) {
    Serial2.println("TMP102 init failed - fan will run at safe duty");
  }

  powerBtn.setOnStateChange(onPiStateChange);
  powerBtn.begin();

  fan.begin();

  // Restore fan config from NVS (set by Pi on previous connection).
  // This ensures the correct curve is applied before the Pi reconnects.
  {
    FanPersistConfig fanNvs{};
    if (loadFanConfig(fanNvs)) {
      fan.setEnabled(fanNvs.enabled);
      fan.setCurve(fanNvs.tOn, fanNvs.tFull);
    }
  }

  // Front-light starts off; applyFrontLight will set the correct level after setupUI.
  pinMode(PIN_FRONT_LIGHT, OUTPUT);
  digitalWrite(PIN_FRONT_LIGHT, LOW);

  SPI.begin(PIN_SCK, -1, PIN_MOSI, PIN_CS);
  display.init(0); // 0 = disable GxEPD2 Serial diagnostics (Serial is used for Pi UART)
  pinMode(PIN_BUSY, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // Rotation: selon ton montage (0/1/2/3). Ajuste si besoin.
  display.setRotation(2);

  // Force: PAS de fast partial update (differential)
  // (GxEPD2 expose ce flag sur le driver)
  //display.epd2.hasFastPartialUpdate;


  touch.begin(PIN_I2C_SDA, PIN_I2C_SCL, PIN_TOUCH_IRQ);
  touch.setDisplayWidth(240);   // invert X so touch matches display (rotation 2)
  touch.setDisplayHeight(display.height());  // invert Y so touch matches display

  setupUI(uiManager, model, onSettingsBackClick, nullptr, &proto);
  applyFrontLight(model.settings.brightness);  // restore saved brightness

  Serial2.println("Boot OK");

  // Let the UART settle and any ROM boot noise flush before sending to Pi.
  delay(1000);
  proto.sendCommand("READY", nullptr, 0);

  // Send firmware version immediately after READY so Pi can detect mismatches.
  AsciiProto::KV verKv[] = { {"ver", ""} };
  strncpy(verKv[0].val, FIRMWARE_VERSION, sizeof(verKv[0].val) - 1);
  proto.sendCommand("VERSION", verKv, 1);
}

void loop() {
  proto.poll();
  powerBtn.update();
  uint32_t now = millis(); // captured after poll so setClock's clockReceivedAtMs <= now
  model.updateClockDisplay(now);

  // Debug commands via Serial2 — wrapped in a lambda so that early return
  // exits the command handler only, not loop() itself. Without this, returning
  // here would skip uiManager.update(), sensor reads, and touch polling.
  if (Serial2.available() > 0)
  {
    [&]() {
      char inputBuf[64];
      int n = (int)Serial2.readBytesUntil('\n', inputBuf, (int)sizeof(inputBuf) - 1);
      while (n > 0 && (inputBuf[n - 1] == '\r' || inputBuf[n - 1] == ' ')) --n;
      inputBuf[n] = '\0';

      if (n == 0) return;

      char cmd = inputBuf[0];

      // Front-light test: f<0-4>  e.g. f0=off, f4=max
      if (cmd == 'f' && n > 1)
      {
        uint8_t level = (uint8_t)constrain(atoi(inputBuf + 1), 0, 4);
        applyFrontLight(level);
        Serial2.printf("[cmd] front-light level=%u/4\n", level);
        return;
      }

      // Raw GPIO test: g0=LOW, g1=HIGH (no LEDC, direct digitalWrite)
      if (cmd == 'g' && n > 1)
      {
        int val = atoi(inputBuf + 1);
        ledcDetach(PIN_FRONT_LIGHT);
        pinMode(PIN_FRONT_LIGHT, OUTPUT);
        digitalWrite(PIN_FRONT_LIGHT, val ? HIGH : LOW);
        Serial2.printf("[cmd] GPIO%d -> %s\n", PIN_FRONT_LIGHT, val ? "HIGH" : "LOW");
        return;
      }

      // TMP102 read: 't' -> print current temperature
      if (cmd == 't' && n == 1)
      {
        float tC = 0.0f;
        if (!s_tmp102Ok) {
          Serial2.println("[cmd] TMP102 not initialized");
        } else if (!tmp102.read(tC)) {
          Serial2.println("[cmd] TMP102 read failed");
        } else {
          Serial2.printf("[cmd] TMP102 = %.2f C\n", tC);
        }
        return;
      }

      // Fan manual control: v<0-255>  e.g. v0=stop, v128=50%, v255=full
      // Bypasses temperature logic; the automatic loop will resume control on next sensor read.
      if (cmd == 'v' && n > 1)
      {
        uint8_t duty = (uint8_t)constrain(atoi(inputBuf + 1), 0, 255);
        if (duty == 0) {
          fan.stop();
        } else {
          ledcWrite(PWM_PIN, duty);
        }
        Serial2.printf("[cmd] fan duty=%u/255\n", duty);
        return;
      }

      // Language: l0 = FR, l1 = EN. Only if language actually changed: save and rebuild.
      if (cmd == 'l' && n > 1)
      {
        uint8_t langId = (uint8_t)constrain(atoi(inputBuf + 1), 0, 1);
        if (langId != model.settings.language) {
          model.setLanguage(langId);
          saveSettingsIfChanged(model);
          rebuildPageAfterLanguageChange(langId, uiManager);
          Serial2.printf("[cmd] language set to %s\n", langId == 0 ? "FR" : "EN");
        }
        return;
      }

      // Test weather icon: w<0-15>  e.g. w12=sunny, w9=rainy, w0=N/A
      if (cmd == 'w' && n > 1)
      {
        uint8_t code = (uint8_t)constrain(atoi(inputBuf + 1), 0, WEATHER_CODE_MAX);
        Serial2.printf("[cmd] weather test - code: %u\n", code);
        model.setWeather(code);
        model.setTempOutX10(225);
        uiManager.update();
        return;
      }

      // Handle single character commands
      switch (cmd)
      {
        case 'c':
          Serial2.println("[cmd] clear");
          break;
        case 's':
          Serial2.println("[cmd] sleep");
          break;
        case 'j':
          model.setNet(true);
          model.setWifi(true);
          model.setExt(true);
          model.setZigbee(true);
          model.setThread(true);
          model.setIr(true);
          model.setMhz433(true);
          model.setMatter(true);
          break;
        case 'p':
          uiManager.navigateTo(PageId::Onboarding);
          break;
        case 'o':
          uiManager.navigateTo(PageId::Home);
          break;
        case 'm':
          uiManager.navigateTo(PageId::Loading);
          break;
        case 'w':
          Serial2.println("[cmd] weather test - usage: w0-w15");
          Serial2.println("  0=N/A, 1=night, 2=cloudy, 3=fog, 4=hail");
          Serial2.println("  5=lightning, 6=lightning-rain, 7=partlycloudy");
          Serial2.println("  8=pouring, 9=rainy, 10=snowy, 11=snowy-rain");
          Serial2.println("  12=sunny, 13=windy, 14=windy-cloudy, 15=alert");
          break;
        default:
          Serial2.println("[cmd] unknown");
          break;
      }
    }();
  }

  // Sensors + model + fan: every 5 s.
  // TMP102 -> box temperature -> fan control.
  // BME280 -> ambient temperature/humidity/pressure -> display + SENS.
  if (now - lastBmeMs >= BME_PERIOD_MS) {
    lastBmeMs = now;

    // TMP102: box temperature drives the fan
    float boxTc = 0.0f;
    if (s_tmp102Ok) {
      if (!tmp102.read(boxTc)) {
        Serial2.println("TMP102 read failed -> fan safe 100%");
        fan.safeMode();
      } else {
        fan.updateFromTemperature(boxTc);
      }
    }

    // BME280: ambient temperature/humidity/pressure for display and Pi
    float tC, pPa, hPct;
    if (bme.read(tC, pPa, hPct)) {
      lastTc = tC;
      lastPPa = pPa;
      lastHPct = hPct;
      model.setTempInX10((int16_t)constrain((int)(tC * 10.0f), -32768, 32767));
      model.setHumidity((uint8_t)constrain((int)hPct, 0, 100));
    }
  }

  // Send sensors every 30 s to Pi (uses last BME values + TMP102 case temp).
  if (powerBtn.getPiState() == PowerButton::PiState::ON) {
    if (now - lastSensMs >= SENS_PERIOD_MS) {
      lastSensMs = now;

      // BME280: ambient temperature, humidity, pressure (only if sensor initialised)
      if (s_bmeOk) {
        snprintf(tBuf, sizeof(tBuf), "%.2f", lastTc);
        snprintf(hBuf, sizeof(hBuf), "%.2f", lastHPct);
        snprintf(pBuf, sizeof(pBuf), "%.0f", lastPPa);
        AsciiProto::KV kv[] = {
          {"tC",  ""}, {"hum", ""}, {"pPa", ""}
        };
        strncpy(kv[0].val, tBuf, sizeof(kv[0].val) - 1);
        strncpy(kv[1].val, hBuf, sizeof(kv[1].val) - 1);
        strncpy(kv[2].val, pBuf, sizeof(kv[2].val) - 1);
        proto.sendCommand("SENS", kv, 3);
      }

      // TMP102: case (box) temperature – separate verb to avoid mixing with BME280
      if (s_tmp102Ok) {
        float caseBoxTc = 0.0f;
        if (tmp102.read(caseBoxTc)) {
          snprintf(boxTcBuf, sizeof(boxTcBuf), "%.2f", caseBoxTc);
          AsciiProto::KV caseKv[] = { {"tC", ""} };
          strncpy(caseKv[0].val, boxTcBuf, sizeof(caseKv[0].val) - 1);
          proto.sendCommand("CASE", caseKv, 1);
        }
      }
    }
  }

  // UI Update (automatic rendering)
  uiManager.update();

  //Serial.printf("T=%.2fC Fan=%s duty=%u/255\n",
  //            tC, fan.isFanActive() ? "ON" : "OFF", fan.currentDuty());

  // Poll touch every loop, with non-blocking cooldown to debounce rapid taps.
  static uint32_t lastTouchMs = 0;
  TouchFT6336::State st;
  if ((now - lastTouchMs >= TOUCH_COOLDOWN_MS) && touch.read(st) && st.count > 0)
  {
    lastTouchMs = now;
    int16_t tx = (int16_t)st.p0.x;
    int16_t ty = (int16_t)st.p0.y;
    Serial2.printf("[touch] x=%d y=%d page=%d\n", tx, ty, (int)uiManager.getCurrentPageId());
    bool consumed = uiManager.handleTouch(tx, ty);
    if (consumed) Serial2.println("[touch] consumed by page");
  }

  delay(20);  // small yield, loop stays responsive
}