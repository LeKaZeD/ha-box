#pragma once
#include <Arduino.h>

class FanController {
public:
  struct Config {
    int pwmPin;
    int pwmFreqHz = 25000;
    int pwmResBits = 8;

    float tFanOff = 25.0f;
    float tFanOn  = 28.0f;
    float tFull   = 35.0f;

    uint8_t dutyMin = 10;
    uint8_t dutyMax = 255;

    uint8_t safeDuty = 255; // en cas d'erreur capteur
  };

  explicit FanController(const Config& cfg);

  void begin();
  void setEnabled(bool enabled);
  bool isEnabled() const;

  // Update temperature curve at runtime (called when Pi sends FAN config).
  // tFanOff is derived as tOn - 3 °C (fixed hysteresis).
  void setCurve(float tOn, float tFull);

  // Appelle ça à chaque loop avec une température valide
  void updateFromTemperature(float tC);

  // Appelle ça si ton capteur est en erreur (lecture impossible)
  void safeMode();

  // Utilitaire: couper le ventilo
  void stop();

  // Lecture état pour debug/UI
  uint8_t currentDuty() const;
  bool isFanActive() const;

private:
  uint8_t mapDuty(float tC) const;
  void setDuty(uint8_t duty);

private:
  Config m_cfg;
  bool m_enabled = true;
  bool m_fanActive = false;
  uint8_t m_duty = 0;
};