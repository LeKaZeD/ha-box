#include "FanController.h"

FanController::FanController(const Config& cfg) : m_cfg(cfg) {}

void FanController::begin() {
  ledcAttach(m_cfg.pwmPin, m_cfg.pwmFreqHz, m_cfg.pwmResBits);
  stop();
}

void FanController::setEnabled(bool enabled) {
  m_enabled = enabled;
  if (!m_enabled) stop();
}

void FanController::setCurve(float tOn, float tFull) {
  if (tFull <= tOn) {
    Serial2.println("[FanCtrl] Invalid curve: tFull <= tOn, ignoring");
    return;
  }
  m_cfg.tFanOn  = tOn;
  m_cfg.tFull   = tFull;
  m_cfg.tFanOff = tOn - 3.0f;
}

bool FanController::isEnabled() const {
  return m_enabled;
}

void FanController::stop() {
  m_fanActive = false;
  setDuty(0);
}

uint8_t FanController::currentDuty() const {
  return m_duty;
}

bool FanController::isFanActive() const {
  return m_fanActive;
}

void FanController::safeMode() {
  if (!m_enabled) return;
  m_fanActive = true;
  setDuty(m_cfg.safeDuty);
}

void FanController::updateFromTemperature(float tC) {
  if (!m_enabled) return;

  // Hysteresis ON/OFF
  if (!m_fanActive) {
    if (tC >= m_cfg.tFanOn) m_fanActive = true;
  } else {
    if (tC <= m_cfg.tFanOff) m_fanActive = false;
  }

  uint8_t duty = m_fanActive ? mapDuty(tC) : 0;
  setDuty(duty);
}

uint8_t FanController::mapDuty(float tC) const {
  if (tC >= m_cfg.tFull) return m_cfg.dutyMax;
  if (tC <= m_cfg.tFanOn) return m_cfg.dutyMin;

  float x = (tC - m_cfg.tFanOn) / (m_cfg.tFull - m_cfg.tFanOn);
  float duty = m_cfg.dutyMin + x * (m_cfg.dutyMax - m_cfg.dutyMin);

  if (duty < 0) duty = 0;
  if (duty > 255) duty = 255;
  return (uint8_t)(duty + 0.5f);
}

void FanController::setDuty(uint8_t duty) {
  m_duty = duty;
  ledcWrite(m_cfg.pwmPin, duty); // ESP32 core 3.x => ledcWrite(pin, duty)
}