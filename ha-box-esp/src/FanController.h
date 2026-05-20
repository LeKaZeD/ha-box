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

    uint8_t safeDuty = 255; // duty used when sensor read fails
  };

  explicit FanController(const Config& cfg);

  void begin();
  void setEnabled(bool enabled);
  bool isEnabled() const;

  // Update temperature curve at runtime (called when Pi sends FAN config).
  // tFanOff is derived as tOn - 3 °C (fixed hysteresis).
  void setCurve(float tOn, float tFull);

  // Call every loop with the current temperature reading.
  void updateFromTemperature(float tC);

  // Call when the temperature sensor read fails; runs fan at safeDuty.
  void safeMode();

  // Force fan off immediately.
  void stop();

  // Current state accessors for debug/UI.
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