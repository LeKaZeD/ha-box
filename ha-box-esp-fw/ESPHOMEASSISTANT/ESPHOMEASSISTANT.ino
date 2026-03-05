#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "settings_persistence.h"
#include "buzzer.h"
#include "BME280.h"
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
    
    // Si on est en loading, passer à Home
    if (uiManager.getCurrentPageId() == PageId::Loading) {
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
  }
  else if (st == PowerButton::PiState::OFF) {
    Serial.println("PiState=OFF");
    model.setNet(false);
    model.setLan(false);
    model.setWifi(false);
    model.setExt(false);
    
    // Retour à la page loading si on était sur Home
    if (uiManager.getCurrentPageId() == PageId::Home) {
      uiManager.navigateTo(PageId::Loading);
    }
  }
  else {
    Serial.println("PiState=SHUTDOWN_PENDING");
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
    Serial.println("BME280 init failed");
    while (1) delay(10);
  }

  powerBtn.setOnStateChange(onPiStateChange);
  powerBtn.begin();

  fan.begin();

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

  // BME + model + fan: every 5 s only
  if (now - lastBmeMs >= BME_PERIOD_MS) {
    lastBmeMs = now;
    float tC, pPa, hPct;
    if (!bme.read(tC, pPa, hPct)) {
      Serial.println("BME read failed -> fan safe 100%");
      fan.safeMode();
    } else {
      lastTc = tC;
      lastPPa = pPa;
      lastHPct = hPct;
      model.setTempInX10((int16_t)(tC * 10.0f));
      model.setHumidity((uint8_t)hPct);
      fan.updateFromTemperature(tC);
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