#include "PowerButton.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

PowerButton* PowerButton::s_instance = nullptr;

PowerButton::PowerButton(const Config& cfg,  AsciiProto& proto)
: m_cfg(cfg), m_proto(proto) {}

void PowerButton::begin() {
  s_instance = this;

  pinMode(m_cfg.btnPin, INPUT_PULLUP);

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

void IRAM_ATTR PowerButton::isrThunk() {
  if (s_instance) s_instance->onBtnEdgeISR();
}

void IRAM_ATTR PowerButton::onBtnEdgeISR() {
  // Note: millis() dans ISR peut marcher, mais pas garanti ISR-safe partout.
  // Tu m'avais demandé de le garder, donc on le conserve.
  uint32_t now = millis();

  if (!m_isrFired) {
    m_edgeMs = now;
    m_isrFired = true;
    return;
  }

  // Deuxième front => on a une durée
  uint32_t dt = now - m_edgeMs;
  m_upDurationMs = dt;

  // On marque un event côté loop (décision ensuite)
  // On marque un event côté loop (décision ensuite)
  if (dt >= m_cfg.longPressMs) {
    m_evLong = true;
  } else if (dt >= m_cfg.minClickMs) {
    m_evClick = true;
  } else {
    // dt trop court => ignore
  }

  m_isrFired = false;
}

void PowerButton::touchHeartbeat() {
  m_lastHbMs = millis();
}

void PowerButton::update() {
  bool click = false;
  bool lng = false;

  // 1) Récupère les events ISR
  noInterrupts();
  if (m_evClick) { click = true; m_evClick = false; }
  if (m_evLong)  { lng   = true; m_evLong  = false; }
  interrupts();

  // 2) Lock bouton pendant shutdown_pending : on ignore click/long
  if (m_piState != PiState::SHUTDOWN_PENDING) {
    if (lng) handleLongPress();
    if (click) handleClick();
  } else {
    // Optionnel : debug
    // if (click || lng) Serial.println("Button locked (shutdown pending)");
  }

  // 3) Toujours exécuter la logique de timeout / état
  uint32_t now = millis();

  // Pi considéré OFF si plus de "vie" pendant hbTimeout (ou activité UART)
  if (m_piState != PiState::OFF && !isPiAliveNow(now)) {
    setPiState(PiState::OFF);
    enterDeepSleep();
    return;
  }

  // Optional: this block is redundant with the one above;
  // keep it if you want the intent to be more explicit.
  if (m_piState == PiState::SHUTDOWN_PENDING && !isPiAliveNow(now)) {
    setPiState(PiState::OFF);
    enterDeepSleep();
    return;
  }
}

bool PowerButton::isPiAliveNow(uint32_t nowMs) const {
  if (m_cfg.hbTimeoutMs == 0) return true;
  return (nowMs - m_lastHbMs) <= m_cfg.hbTimeoutMs;
}

void PowerButton::onUartLine(const char* line) {
  if (!line || !line[0]) return;

  // Toute activité UART peut être considérée comme heartbeat
  m_lastHbMs = millis();

  if (strcmp(line, "HB") == 0) {
    // heartbeat explicite
    return;
  }

  if (strcmp(line, "HALTED") == 0 || strcmp(line, "PI_HALTED") == 0) {
    setPiState(PiState::OFF);
    enterDeepSleep();
    return;
  }
}

void PowerButton::handleClick() {
  m_lastActionMs = millis();

  if (m_piState == PiState::ON) {
    requestShutdown();
    return;
  }

  // Pi OFF: on réveille le Pi via J2
  pulseJ2();
  setPiState(PiState::ON);
  m_lastHbMs = millis(); // on évite de tomber OFF immédiat pendant boot
}

void PowerButton::handleLongPress() {
  m_lastActionMs = millis();

  if (m_piState == PiState::ON) {
    // Appui long = action "hard" via J2
    pulseJ2();
  }
}

void PowerButton::requestShutdown() {
  setPiState(PiState::SHUTDOWN_PENDING);

  // Commande whitelistée vers ton bridge HAOS
  // Format attendu: TOKEN <secret> shutdown
  bool ok = m_proto.sendWithAck("SHUTDOWN_REQUEST", nullptr, 0, 500, 3);
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

  // ext0: un seul GPIO RTC. Vérifie que BTN_PIN est RTC-capable (GPIO33 l'est).
  esp_sleep_enable_ext0_wakeup((gpio_num_t)m_cfg.btnPin, 0);

  // Met J2 dans un état safe avant sleep
  if (m_cfg.j2ActiveHigh) digitalWrite(m_cfg.j2Pin, LOW);
  else digitalWrite(m_cfg.j2Pin, HIGH);

  // Optionnel: isolation RTC hold
  rtc_gpio_hold_dis((gpio_num_t)m_cfg.j2Pin);

  esp_deep_sleep_start();
}