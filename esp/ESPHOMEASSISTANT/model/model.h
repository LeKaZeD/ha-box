#pragma once
#include <stdint.h>
#include <stddef.h>

#include "dirty_key.h"
#include "state_onboarding.h"
#include "state_loading.h"
#include "state_home.h"
#include "state_settings.h"

struct Model
{
  OnboardingState onboarding;
  LoadingState loading;
  HomeState home;
  SettingsState settings;

  // Dirty bitmask: set by setters, consumed by UI
  uint32_t dirty = DK_None;

  // ---- Helpers ----
  bool hasDirty() const { return dirty != DK_None; }
  uint32_t takeDirty();

  // ---- Onboarding setters ----
  void setOnboardingActive(bool active);
  void setOnboardingQrPayload(const char* payload); // string
  void setOnboardingLink(const char* link);         // string

  // ---- Loading setters ----
  void setLoadingActive(bool active);
  void setLoadingReason(const char* reason);
  void setLoadingAnimFrame(uint8_t frame);

  // ---- Home setters ----
  void setWifi(bool wifi_ok);
  void setBLE(bool ble_ok);
  void setZigbee(bool zigbee_ok);
  void setThread(bool thread_ok);
  void setIr(bool ir_ok);
  void setMhz433(bool mhz433_ok);
  void setMatter(bool matter_ok);
  void setTempOutX10(int16_t v);
  void setTempInX10(int16_t v);
  void setWeather(uint8_t code);
  void setHumidity(uint8_t pct);
  void setClock(uint8_t hh, uint8_t mm, uint8_t ss, uint32_t receivedAtMs);
  void updateClockDisplay(uint32_t nowMs);
  void setDayMode(uint8_t mode);

  // ---- Settings setters ----
  void setBrightness(uint8_t level);
  void setSoundEnabled(bool enabled);
  void setLanguage(uint8_t lang);
};