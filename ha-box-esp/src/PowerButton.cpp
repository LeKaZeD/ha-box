#include "PowerButton.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"

PowerButton* PowerButton::s_instance = nullptr;

PowerButton::PowerButton(const Config& cfg,  AsciiProto& proto)
: m_cfg(cfg), m_proto(proto) {}

void PowerButton::begin() {
  s_instance = this;

  pinMode(m_cfg.btnPin, INPUT);

  if (m_cfg.piAlivePin >= 0) {
    pinMode(m_cfg.piAlivePin, INPUT_PULLDOWN);
    Serial2.printf("[PowerBtn] Pi alive pin: GPIO%d\n", m_cfg.piAlivePin);
  }

  pinMode(m_cfg.j2Pin, OUTPUT);
  if (m_cfg.j2ActiveHigh) digitalWrite(m_cfg.j2Pin, LOW);
  else digitalWrite(m_cfg.j2Pin, HIGH);

  attachInterrupt(digitalPinToInterrupt(m_cfg.btnPin), PowerButton::isrThunk, CHANGE);

  m_lastHbMs = millis();
  m_lastActionMs = millis();
  pulseJ2();
}

void PowerButton::setOnStateChange(void (*cb)(PiState st)) {
  m_onStateChange = cb;
}

void PowerButton::setPiState(PiState st) {
  m_piState = st;
  if (m_onStateChange) m_onStateChange(m_piState);
}

PowerButton::PiState PowerButton::getPiState() const {
  return m_piState;
}

void PowerButton::isrThunk() {
  if (s_instance) s_instance->onBtnEdgeISR();
}

void PowerButton::onBtnEdgeISR() {
  // esp_timer_get_time() is explicitly ISR-safe (ESP-IDF docs); returns µs.
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000LL);

  if (!m_isrFired) {
    m_edgeMs = now;
    m_isrFired = true;
    return;
  }

  // Second edge: compute press duration
  uint32_t dt = now - m_edgeMs;
  m_upDurationMs = dt;

  // Classification: click 120ms-2s, long press >=5s, entre 2s-5s => ignore
  if (dt >= m_cfg.longPressMs) {
    m_evLong = true;
  } else if (dt >= m_cfg.minClickMs && dt <= m_cfg.maxClickMs) {
    m_evClick = true;
  }
  // dt trop court ou entre maxClick et longPress => ignore

  m_isrFired = false;
}

void PowerButton::touchHeartbeat() {
  m_lastHbMs = millis();
}

void PowerButton::update() {
  bool click = false;
  bool lng = false;

  // 1) Collect ISR events
  noInterrupts();
  if (m_evClick) { click = true; m_evClick = false; }
  if (m_evLong)  { lng   = true; m_evLong  = false; }
  interrupts();

  // 2) Pi alive GPIO: if LOW while Pi should be ON → Pi has shut down
  if (m_cfg.piAlivePin >= 0 && m_piState != PiState::OFF) {
    if (digitalRead(m_cfg.piAlivePin) == LOW) {
      Serial2.println("[PowerBtn] Pi alive pin LOW → Pi is off");
      setPiState(PiState::OFF);
      enterDeepSleep();
      return;
    }
  }

  // 3) Lock bouton pendant shutdown_pending / halting : on ignore click/long
  if (m_piState != PiState::SHUTDOWN_PENDING && m_piState != PiState::HALTING) {
    if (lng) handleLongPress();
    if (click) handleClick();
  } else {
    // Optionnel : debug
    // if (click || lng) Serial.println("Button locked (shutdown pending)");
  }

  // 3) Always run timeout / state logic
  uint32_t now = millis();

  // Pi considered OFF if no heartbeat within the timeout for the current state
  if (m_piState != PiState::OFF) {
    uint32_t timeoutMs;
    if (m_piState == PiState::SHUTDOWN_PENDING)
      timeoutMs = m_cfg.shutdownPendingTimeoutMs;
    else if (m_piState == PiState::HALTING)
      timeoutMs = m_cfg.haltingTimeoutMs;
    else
      timeoutMs = m_cfg.hbTimeoutMs;

    if ((now - m_lastHbMs) > timeoutMs) {
      setPiState(PiState::OFF);
      enterDeepSleep();
      return;
    }
  }
}


void PowerButton::onUartLine(const char* line) {
  if (!line || !line[0]) return;

  // Any UART activity counts as a heartbeat
  m_lastHbMs = millis();

  if (strstr(line, "HB")) return;  // heartbeat explicite

  // SHUTDOWN_ACCEPTED: add-on acknowledged shutdown → wait for heartbeat silence
  // HALTED: Pi halted (legacy compat)
  if (strstr(line, "SHUTDOWN_ACCEPTED") || strstr(line, "HALTED")) {
    setPiState(PiState::HALTING);
    return;
  }
}

void PowerButton::handleClick() {
  m_lastActionMs = millis();

  if (m_piState == PiState::ON) {
    requestShutdown();
    return;
  }

  // Pi is OFF: wake it up via J2
  pulseJ2();
  setPiState(PiState::ON);
  m_lastHbMs = millis(); // prevent immediate OFF timeout during Pi boot
}

void PowerButton::handleLongPress() {
  m_lastActionMs = millis();

  if (m_piState == PiState::ON) {
    // Appui long = hard stop via J2 (5.5s pulse)
    resetJ2();
  }
}

void PowerButton::requestShutdown() {
  setPiState(PiState::SHUTDOWN_PENDING);
  m_proto.sendCommand("SHUTDOWN_REQUEST", nullptr, 0);
}

void PowerButton::pulseJ2() {
  if (m_cfg.j2ActiveHigh) {
    digitalWrite(m_cfg.j2Pin, HIGH);
    delay(m_cfg.j2PulseMs);
    digitalWrite(m_cfg.j2Pin, LOW);
  } else {
    digitalWrite(m_cfg.j2Pin, LOW);
    delay(m_cfg.j2PulseMs);
    digitalWrite(m_cfg.j2Pin, HIGH);
  }
}

void PowerButton::resetJ2() {
  if (m_cfg.j2ActiveHigh) {
    digitalWrite(m_cfg.j2Pin, HIGH);
    delay(m_cfg.j2ResetMs);
    digitalWrite(m_cfg.j2Pin, LOW);
  } else {
    digitalWrite(m_cfg.j2Pin, LOW);
    delay(m_cfg.j2ResetMs);
    digitalWrite(m_cfg.j2Pin, HIGH);
  }
}

void PowerButton::enterDeepSleep() {
  // Configure wake sur bouton (active LOW en pullup)
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // ext0: single RTC GPIO wakeup source. BTN_PIN must be RTC-capable (GPIO33 is).
  esp_sleep_enable_ext0_wakeup((gpio_num_t)m_cfg.btnPin, 0);

  // Drive J2 to idle state before entering deep sleep
  if (m_cfg.j2ActiveHigh) digitalWrite(m_cfg.j2Pin, LOW);
  else digitalWrite(m_cfg.j2Pin, HIGH);

  // Optionnel: isolation RTC hold
  rtc_gpio_hold_dis((gpio_num_t)m_cfg.j2Pin);

  esp_deep_sleep_start();
}