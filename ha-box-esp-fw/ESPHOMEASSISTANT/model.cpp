#include "model/model.h"
#include <string.h>

// -------- Internal helpers --------

static bool safeCopy(char* dst, size_t dstSize, const char* src)
{
  if (!dst || dstSize == 0) return false;
  if (!src) src = "";

  // Quick compare: if identical, do not touch
  if (strncmp(dst, src, dstSize) == 0) {
    // we always keep dst null-terminated here.
    return false;
  }

  // Copy + null-terminate
  size_t n = strnlen(src, dstSize - 1);
  memcpy(dst, src, n);
  dst[n] = '\0';
  return true;
}

// -------- Model --------

uint32_t Model::takeDirty()
{
  uint32_t d = dirty;
  dirty = DK_None;
  return d;
}

// -------- Onboarding --------

void Model::setOnboardingActive(bool active)
{
  if (onboarding.active == active) return;
  onboarding.active = active;
  // Si tu veux, tu peux créer DK_OB_Active. Là, on force redraw QR+link.
  dirty |= (DK_OB_QRPayload | DK_OB_Link);
}

void Model::setOnboardingQrPayload(const char* payload)
{
  if (safeCopy(onboarding.qr_payload, sizeof(onboarding.qr_payload), payload)) {
    dirty |= DK_OB_QRPayload;
  }
}

void Model::setOnboardingLink(const char* link)
{
  if (safeCopy(onboarding.link, sizeof(onboarding.link), link)) {
    dirty |= DK_OB_Link;
  }
}

// -------- Loading --------

void Model::setLoadingActive(bool active)
{
  if (loading.active == active) return;
  loading.active = active;
  dirty |= DK_LD_Active;
}

void Model::setLoadingReason(const char* reason)
{
  if (safeCopy(loading.reason, sizeof(loading.reason), reason)) {
    dirty |= DK_LD_Reason;
  }
}

void Model::setLoadingAnimFrame(uint8_t frame)
{
  if (loading.anim_frame == frame) return;
  loading.anim_frame = frame;
  dirty |= DK_LD_Anim;
}

// -------- Home --------

void Model::setCore(bool ok)
{
  if (home.core_ok == ok) return;
  home.core_ok = ok;
}

void Model::setSup(bool ok)
{
  if (home.sup_ok == ok) return;
  home.sup_ok = ok;
}

void Model::setNet(bool ok)
{
  if (home.net_ok == ok) return;
  home.net_ok = ok;
  dirty |= DK_HM_Wifi;
}

void Model::setLan(bool ok)
{
  if (home.lan_ok == ok) return;
  home.lan_ok = ok;
  dirty |= DK_HM_Wifi;
}

void Model::setWifi(bool wifi_ok)
{
  if (home.wifi_ok == wifi_ok) return;
  home.wifi_ok = wifi_ok;
  dirty |= DK_HM_Wifi;
}

void Model::setExt(bool ok)
{
  if (home.ext_ok == ok) return;
  home.ext_ok = ok;
  dirty |= DK_HM_Ext;
}

void Model::setBLE(bool ble_ok)
{
  if (home.ble_ok == ble_ok) return;
  home.ble_ok = ble_ok;
}

void Model::setZigbee(bool zigbee_ok)
{
  if (home.zigbee_ok == zigbee_ok) return;
  home.zigbee_ok = zigbee_ok;
  dirty |= DK_HM_Zigbee;
}

void Model::setThread(bool thread_ok)
{
  if (home.thread_ok == thread_ok) return;
  home.thread_ok = thread_ok;
  dirty |= DK_HM_Thread;
}

void Model::setIr(bool ir_ok)
{
  if (home.ir_ok == ir_ok) return;
  home.ir_ok = ir_ok;
  dirty |= DK_HM_Ir;
}

void Model::setMhz433(bool mhz433_ok)
{
  if (home.mhz433_ok == mhz433_ok) return;
  home.mhz433_ok = mhz433_ok;
  dirty |= DK_HM_Mhz433;
}

void Model::setMatter(bool matter_ok)
{
  if (home.matter_ok == matter_ok) return;
  home.matter_ok = matter_ok;
  dirty |= DK_HM_Matter;
}

void Model::setTempOutX10(int16_t v)
{
  if (home.temp_out_x10 == v) return;
  home.temp_out_x10 = v;
  dirty |= DK_HM_TempOut;
}

void Model::setTempInX10(int16_t v)
{
  if (home.temp_in_x10 == v) return;
  home.temp_in_x10 = v;
  dirty |= DK_HM_TempIn;
}

void Model::setWeather(uint8_t code)
{
  if (home.weather_code == code) return;
  home.weather_code = code;
  dirty |= DK_HM_Weather;
}

void Model::setHumidity(uint8_t pct)
{
  if (home.humidity == pct) return;
  home.humidity = pct;
  dirty |= DK_HM_Humidity;
}

void Model::setClock(uint8_t hh, uint8_t mm, uint8_t ss, uint32_t receivedAtMs)
{
  home.hour = hh;
  home.minute = mm;
  home.second = ss;
  home.clockReceivedAtMs = receivedAtMs;
  home.displayHour = hh;
  home.displayMinute = mm;
  home.displaySecond = ss;
  dirty |= DK_HM_Clock;
}

void Model::updateClockDisplay(uint32_t nowMs)
{
  if (home.clockReceivedAtMs == 0) return;
  uint32_t elapsedSec = (nowMs - home.clockReceivedAtMs) / 1000u;
  uint32_t totalSec = (uint32_t)home.hour * 3600u + (uint32_t)home.minute * 60u
                    + (uint32_t)home.second + elapsedSec;
  uint8_t h = (uint8_t)((totalSec / 3600u) % 24u);
  uint8_t m = (uint8_t)((totalSec / 60u) % 60u);
  uint8_t s = (uint8_t)(totalSec % 60u);
  if (home.displayHour != h || home.displayMinute != m) {
    home.displayHour = h;
    home.displayMinute = m;
    home.displaySecond = s;
    dirty |= DK_HM_Clock;
  } else {
    home.displaySecond = s;
  }
}

void Model::setDayMode(uint8_t mode)
{
  if (home.day_mode == mode) return;
  home.day_mode = mode;
  dirty |= DK_HM_DayMode;
}

void Model::setBrightness(uint8_t level)
{
  if (level > 4) level = 4;
  if (settings.brightness == level) return;
  settings.brightness = level;
  dirty |= DK_ST_Brightness;
}

void Model::setSoundEnabled(bool enabled)
{
  if (settings.sound_enabled == enabled) return;
  settings.sound_enabled = enabled;
  dirty |= DK_ST_Sound;
}

void Model::setLanguage(uint8_t lang)
{
  if (settings.language == lang) return;
  settings.language = lang;
  dirty |= DK_ST_Language;
}