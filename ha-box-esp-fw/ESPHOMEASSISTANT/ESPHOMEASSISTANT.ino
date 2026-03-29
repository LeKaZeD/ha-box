#include <Arduino.h>
#include <Wire.h>
#include "driver/rtc_io.h"
#include "config.h"
#include "settings_persistence.h"
#include "buzzer.h"
#include "BME280.h"
#if ENABLE_TMP102
#include "TMP102.h"
#endif
#include "POWERBUTTON.h"
#include "FANCONTROLLER.h"


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

AsciiProto proto(Serial2);

BME280Min bme(0x76);
#if ENABLE_TMP102
TMP102    tmp102(TMP102_ADDR);
static bool s_tmp102Ok = false;
#endif

PowerButton::Config pbCfg{
  .btnPin = BTN_PIN,
  .j2Pin  = J2_PULSE_PIN,
  .j2ActiveHigh = true,
  .j2PulseMs = 150,
  .debounceMs = 30,
  .minClickMs = 120,
  .maxClickMs = 2000,
  .longPressMs = 5000,
  .hbTimeoutMs = 300000,
  .shutdownPendingTimeoutMs = 60000,
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
  // Allowlist des commandes autorisées depuis le Pi
  if (strcmp(msg.verb, "READY") == 0) return true;
  if (strcmp(msg.verb, "HALTED") == 0) return true;
  if (strcmp(msg.verb, "SHUTDOWN_ACCEPTED") == 0) return true;
  if (strcmp(msg.verb, "STATUS") == 0) return true;
  if (strcmp(msg.verb, "WEATHER") == 0) return true;
  if (strcmp(msg.verb, "CLOCK") == 0) return true;
  if (strcmp(msg.verb, "LANG") == 0) return true;
  *err = "unknown_cmd";
  return false;
}

static void onMsg(const AsciiProto::Message& msg, void* user) {
  Serial.printf("[RX] id=%lu verb=%s\n", (unsigned long)msg.id, msg.verb);

  for (uint8_t i = 0; i < msg.kvCount; i++) {
    Serial.printf("  %s=%s\n", msg.kv[i].key, msg.kv[i].val);
  }
  powerBtn.touchHeartbeat();

  if (strcmp(msg.verb, "READY") == 0) {
    powerBtn.setPiState(PowerButton::PiState::ON);
    model.setLoadingReason(i18n::loading().reasonDefault);
    const PageId cur = uiManager.getCurrentPageId();
    if (cur == PageId::Loading || cur == PageId::Shutdown) {
      uiManager.navigateTo(PageId::Home);
    }
    return;
  }

  if (strcmp(msg.verb, "HALTED") == 0) {
    powerBtn.setPiState(PowerButton::PiState::OFF);
    model.setNet(false);
    model.setLan(false);
    model.setWifi(false);
    model.setExt(false);
    fan.setEnabled(false);
    uiManager.navigateTo(PageId::Shutdown);
    return;
  }
  
  // WEATHER: weather code + outdoor temp
  if (strcmp(msg.verb, "WEATHER") == 0) {
    for (uint8_t i = 0; i < msg.kvCount; i++) {
      if (strcmp(msg.kv[i].key, "code") == 0) {
        model.setWeather((uint8_t)atoi(msg.kv[i].val));
      }
      else if (strcmp(msg.kv[i].key, "tOut") == 0) {
        model.setTempOutX10((int16_t)atoi(msg.kv[i].val));
      }
    }
    return;
  }
  
  // CLOCK: heure + minute + seconde (avance locale entre deux syncs)
  if (strcmp(msg.verb, "CLOCK") == 0) {
    uint8_t hh = 0, mm = 0, ss = 0;
    for (uint8_t i = 0; i < msg.kvCount; i++) {
      if (strcmp(msg.kv[i].key, "hh") == 0) hh = (uint8_t)atoi(msg.kv[i].val);
      if (strcmp(msg.kv[i].key, "mm") == 0) mm = (uint8_t)atoi(msg.kv[i].val);
      if (strcmp(msg.kv[i].key, "ss") == 0) ss = (uint8_t)atoi(msg.kv[i].val);
    }
    model.setClock(hh, mm, ss, millis());
    return;
  }

  // LANG: 0 = français, 1 = anglais (from Pi or other). Only if language actually changed: save and rebuild.
  if (strcmp(msg.verb, "LANG") == 0) {
    for (uint8_t i = 0; i < msg.kvCount; i++) {
      if (strcmp(msg.kv[i].key, "id") == 0) {
        uint8_t langId = (uint8_t)atoi(msg.kv[i].val);
        if (langId > 1) langId = 1;
        if (langId == model.settings.language) return;
        model.setLanguage(langId);
        saveSettingsIfChanged(model);
        rebuildPageAfterLanguageChange(langId, uiManager);
        Serial.printf("[cmd] LANG set to %s (id=%u)\n", langId == 0 ? "FR" : "EN", (unsigned)langId);
        break;
      }
    }
    return;
  }

  // STATUS: optional KV (core, sup, net, lan, wifi, ext, zigbee, thread, matter). If no KV, heartbeat only.
  if (strcmp(msg.verb, "STATUS") == 0) {
    if (msg.kvCount > 0) {
      for (uint8_t i = 0; i < msg.kvCount; i++) {
        bool ok = (atoi(msg.kv[i].val) != 0);
        if (strcmp(msg.kv[i].key, "core") == 0) model.setCore(ok);
        else if (strcmp(msg.kv[i].key, "sup") == 0) model.setSup(ok);
        else if (strcmp(msg.kv[i].key, "net") == 0) model.setNet(ok);
        else if (strcmp(msg.kv[i].key, "lan") == 0) model.setLan(ok);
        else if (strcmp(msg.kv[i].key, "wifi") == 0) model.setWifi(ok);
        else if (strcmp(msg.kv[i].key, "ext") == 0) model.setExt(ok);
        else if (strcmp(msg.kv[i].key, "zigbee") == 0) model.setZigbee(ok);
        else if (strcmp(msg.kv[i].key, "thread") == 0) model.setThread(ok);
        else if (strcmp(msg.kv[i].key, "matter") == 0) model.setMatter(ok);
      }
    }
    return;
  }
}

static void onRawLine(const char* line, void* user) {
  Serial.printf("[RX raw] %s\n", line);
  powerBtn.onUartLine(line);
}

static void onSettingsBackClick(void* user) {
  (void)user;
  saveSettingsIfChanged(model);
  setupUI_rebuildAndGoHome(uiManager, model);
}

static void onAck(const AsciiProto::Ack& ack, void* user) {
  Serial.printf("[RX] ACK id=%lu %s %s\n",
              (unsigned long)ack.id,
              ack.ok ? "OK" : "ERR",
              ack.ok ? "" : ack.err);
  powerBtn.touchHeartbeat();
}

static void onPiStateChange(PowerButton::PiState st) {
  // Log simple
  if (st == PowerButton::PiState::ON) {
    Serial.println("PiState=ON");
    model.setWifi(true);
    // Release the RTC hold set before deep sleep so LEDC can drive the pin again.
    rtc_gpio_hold_dis((gpio_num_t)PWM_PIN);
    fan.setEnabled(true);
  }
  else if (st == PowerButton::PiState::OFF) {
    Serial.println("PiState=OFF");
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
  else {
    Serial.println("PiState=SHUTDOWN_PENDING");
    // Show loading page immediately with shutdown reason so the screen updates
    // without waiting for HALTED (which can take up to shutdownPendingTimeoutMs).
    model.setLoadingReason(i18n::loading().reasonShutdown);
    uiManager.navigateTo(PageId::Loading);
  }
}



void setup() {
  Serial.begin(115200);
  delay(200);

  Serial2.begin(115200, SERIAL_8N1, UART2_RX, UART2_TX);

  proto.begin();
  proto.setRawLineCallback(onRawLine, nullptr);
  proto.setAuthorizeCallback(authorizeIncoming, nullptr);
  proto.setMessageCallback(onMsg, nullptr);
  proto.setAckCallback(onAck, nullptr);

  bme.beginWire(Wire, 21, 22, 100000);
  if (!bme.begin()) {
    Serial.println("BME280 init failed - continuing without ambient sensor");
  }

#if ENABLE_TMP102
  tmp102.beginWire(Wire);
  s_tmp102Ok = tmp102.begin();
  if (!s_tmp102Ok) {
    Serial.println("TMP102 init failed - fan will use BME280 temperature or safe mode");
  }
#endif

  powerBtn.setOnStateChange(onPiStateChange);
  powerBtn.begin();

  fan.begin();

  // Front-light starts off; applyFrontLight will set the correct level after setupUI.
  pinMode(PIN_FRONT_LIGHT, OUTPUT);
  digitalWrite(PIN_FRONT_LIGHT, LOW);

  SPI.begin(PIN_SCK, -1, PIN_MOSI, PIN_CS);
  display.init(115200);
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

  Serial.println("Boot OK");
  
  // Send READY to Pi
  delay(1000);
  proto.sendCommand("READY", nullptr, 0);
}

void loop() {
  uint32_t now = millis();
  proto.poll();
  powerBtn.update();
  model.updateClockDisplay(now);

  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() == 0) return;
    
    char cmd = input.charAt(0);
    
    // Front-light test: f<0-4>  e.g. f0=off, f4=max
    if (cmd == 'f' && input.length() > 1)
    {
      uint8_t level = (uint8_t)constrain(input.substring(1).toInt(), 0, 4);
      applyFrontLight(level);
      Serial.printf("[cmd] front-light level=%u/4\n", level);
      return;
    }

    // Raw GPIO test: g0=LOW, g1=HIGH (no LEDC, direct digitalWrite)
    if (cmd == 'g' && input.length() > 1)
    {
      int val = input.substring(1).toInt();
      ledcDetach(PIN_FRONT_LIGHT);
      pinMode(PIN_FRONT_LIGHT, OUTPUT);
      digitalWrite(PIN_FRONT_LIGHT, val ? HIGH : LOW);
      Serial.printf("[cmd] GPIO%d -> %s\n", PIN_FRONT_LIGHT, val ? "HIGH" : "LOW");
      return;
    }

    // TMP102 read: 't' -> print current temperature
#if ENABLE_TMP102
    if (cmd == 't' && input.length() == 1)
    {
      float tC = 0.0f;
      if (!s_tmp102Ok) {
        Serial.println("[cmd] TMP102 not initialized");
      } else if (!tmp102.read(tC)) {
        Serial.println("[cmd] TMP102 read failed");
      } else {
        Serial.printf("[cmd] TMP102 = %.2f C\n", tC);
      }
      return;
    }
#endif

    // Fan manual control: v<0-255>  e.g. v0=stop, v128=50%, v255=full
    // Bypasses temperature logic; the automatic loop will resume control on next sensor read.
    if (cmd == 'v' && input.length() > 1)
    {
      uint8_t duty = (uint8_t)constrain(input.substring(1).toInt(), 0, 255);
      if (duty == 0) {
        fan.stop();
      } else {
        ledcWrite(PWM_PIN, duty);
      }
      Serial.printf("[cmd] fan duty=%u/255\n", duty);
      return;
    }

    // Language: l0 = FR, l1 = EN. Only if language actually changed: save and rebuild.
    if (cmd == 'l' && input.length() > 1)
    {
      uint8_t langId = (uint8_t)input.substring(1).toInt();
      if (langId > 1) langId = 1;
      if (langId != model.settings.language) {
        model.setLanguage(langId);
        saveSettingsIfChanged(model);
        rebuildPageAfterLanguageChange(langId, uiManager);
        Serial.printf("[cmd] language set to %s\n", langId == 0 ? "FR" : "EN");
      }
      return;
    }

    // Handle commands with parameters (e.g., "w12", "w0")
    if (cmd == 'w' && input.length() > 1)
    {
      // Test weather icon: 'w' + number (0-15)
      // e.g. w12 = sunny, w9 = rainy, w0 = N/A
      String codeStr = input.substring(1);
      uint8_t code = (uint8_t)codeStr.toInt();
      if (code > 15) code = 15;
      
      Serial.print("[cmd] weather test - code: ");
      Serial.println(code);
      
      // Set weather code and temp (for testing)
      model.setWeather(code);
      model.setTempOutX10(225); // 22.5°C for testing
      
      // Force UI update
      uiManager.update();
      return;
    }
    
    // Handle single character commands
    switch (cmd)
    {
      case 'c':
        Serial.println("[cmd] clear");
        break;
      case 's':
        Serial.println("[cmd] sleep");
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
        Serial.println("[cmd] weather test - usage: w0-w15");
        Serial.println("  0=N/A, 1=night, 2=cloudy, 3=fog, 4=hail");
        Serial.println("  5=lightning, 6=lightning-rain, 7=partlycloudy");
        Serial.println("  8=pouring, 9=rainy, 10=snowy, 11=snowy-rain");
        Serial.println("  12=sunny, 13=windy, 14=windy-cloudy, 15=alert");
        break;
      default:
        Serial.println("[cmd] unknown");
        break;
    }
  }

  // Sensors + model + fan: every 5 s.
  // TMP102 -> box temperature -> fan control.
  // BME280 -> ambient temperature/humidity/pressure -> display + SENS.
  if (now - lastBmeMs >= BME_PERIOD_MS) {
    lastBmeMs = now;

#if ENABLE_TMP102
    // TMP102: box temperature drives the fan
    float boxTc = 0.0f;
    if (s_tmp102Ok) {
      if (!tmp102.read(boxTc)) {
        Serial.println("TMP102 read failed -> fan safe 100%");
        fan.safeMode();
      } else {
        fan.updateFromTemperature(boxTc);
      }
    }
#endif

    // BME280: ambient temperature/humidity/pressure for display and Pi
    float tC, pPa, hPct;
    if (bme.read(tC, pPa, hPct)) {
      lastTc = tC;
      lastPPa = pPa;
      lastHPct = hPct;
      model.setTempInX10((int16_t)(tC * 10.0f));
      model.setHumidity((uint8_t)hPct);
#if !ENABLE_TMP102
      fan.updateFromTemperature(tC);
#endif
    } else {
#if !ENABLE_TMP102
      Serial.println("BME read failed -> fan safe 100%");
      fan.safeMode();
#endif
    }
  }

  // Send sensors every 30 s to Pi (uses last BME values)
  if (powerBtn.getPiState() == PowerButton::PiState::ON) {
    if (now - lastSensMs >= SENS_PERIOD_MS) {
      lastSensMs = now;
      snprintf(tBuf, sizeof(tBuf), "%.2f", lastTc);
      snprintf(hBuf, sizeof(hBuf), "%.2f", lastHPct);
      snprintf(pBuf, sizeof(pBuf), "%.0f", lastPPa);
      AsciiProto::KV kv[] = {
        {"tC",  ""}, {"hum", ""}, {"pPa", ""}
      };
      strncpy(kv[0].val, tBuf, sizeof(kv[0].val) - 1);
      strncpy(kv[1].val, hBuf, sizeof(kv[1].val) - 1);
      strncpy(kv[2].val, pBuf, sizeof(kv[2].val) - 1);
      bool ackOk = proto.sendWithAck("SENS", kv, 3, 1000, 3);
      if (!ackOk) Serial.println("SENS: No ACK from Pi");
    }
  }

  // UI Update (automatic rendering)
  uiManager.update();

  //Serial.printf("T=%.2fC Fan=%s duty=%u/255\n",
  //            tC, fan.isFanActive() ? "ON" : "OFF", fan.currentDuty());

  // Poll touch every loop
  TouchFT6336::State st;
  if (touch.read(st) && st.count > 0)
  {
    int16_t tx = (int16_t)st.p0.x;
    int16_t ty = (int16_t)st.p0.y;
    Serial.printf("[touch] x=%d y=%d page=%d\n", tx, ty, (int)uiManager.getCurrentPageId());
    bool consumed = uiManager.handleTouch(tx, ty);
    if (consumed) Serial.println("[touch] consumed by page");
    delay(200);
  }

  delay(20);  // small yield, loop stays responsive
}