#include "settings_persistence.h"
#include "model/model.h"
#include <Arduino.h>
#include <Preferences.h>

static const char* SETTINGS_NAMESPACE = "habox";
static Preferences s_prefs;
static uint8_t s_lastBrightness = 255;
static bool s_lastSoundEnabled = true;
static uint8_t s_lastLanguage = 255;

void loadSettings(Model& model)
{
  if (!s_prefs.begin(SETTINGS_NAMESPACE, true)) {
    Serial.println("[NVS] loadSettings: begin failed");
    return;
  }
  if (s_prefs.isKey("brightness")) {
    uint8_t v = s_prefs.getUChar("brightness", 2);
    model.setBrightness(v);
    s_lastBrightness = v;
  }
  if (s_prefs.isKey("sound")) {
    bool v = s_prefs.getBool("sound", true);
    model.setSoundEnabled(v);
    s_lastSoundEnabled = v;
  }
  if (s_prefs.isKey("lang")) {
    uint8_t v = s_prefs.getUChar("lang", 0);
    model.setLanguage(v);
    s_lastLanguage = v;
  }
  s_prefs.end();
  s_lastBrightness = model.settings.brightness;
  s_lastSoundEnabled = model.settings.sound_enabled;
  s_lastLanguage = model.settings.language;
}

void saveSettingsIfChanged(const Model& model)
{
  const bool changed = (model.settings.brightness != s_lastBrightness) ||
                       (model.settings.sound_enabled != s_lastSoundEnabled) ||
                       (model.settings.language != s_lastLanguage);
  if (!changed) return;
  s_lastBrightness = model.settings.brightness;
  s_lastSoundEnabled = model.settings.sound_enabled;
  s_lastLanguage = model.settings.language;
  if (!s_prefs.begin(SETTINGS_NAMESPACE, false)) {
    Serial.println("[NVS] saveSettingsIfChanged: begin failed");
    return;
  }
  s_prefs.putUChar("brightness", s_lastBrightness);
  s_prefs.putBool("sound", s_lastSoundEnabled);
  s_prefs.putUChar("lang", s_lastLanguage);
  s_prefs.end();
}

// ---------------------------------------------------------------------------
// Fan configuration persistence
// Keys: fan_en (bool), fan_ton (float), fan_tfl (float)
// ---------------------------------------------------------------------------

bool loadFanConfig(FanPersistConfig& out)
{
  if (!s_prefs.begin(SETTINGS_NAMESPACE, true)) {
    Serial.println("[NVS] loadFanConfig: begin failed");
    return false;
  }
  const bool exists = s_prefs.isKey("fan_en");
  if (exists) {
    out.enabled = s_prefs.getBool("fan_en",  false);
    out.tOn     = s_prefs.getFloat("fan_ton", 28.0f);
    out.tFull   = s_prefs.getFloat("fan_tfl", 60.0f);
  }
  s_prefs.end();
  if (exists) {
    Serial.printf("[NVS] fan loaded: en=%d tOn=%.1f tFull=%.1f\n",
                  (int)out.enabled, out.tOn, out.tFull);
  }
  return exists;
}

void saveFanConfig(const FanPersistConfig& cfg)
{
  if (!s_prefs.begin(SETTINGS_NAMESPACE, false)) {
    Serial.println("[NVS] saveFanConfig: begin failed");
    return;
  }
  s_prefs.putBool("fan_en",   cfg.enabled);
  s_prefs.putFloat("fan_ton", cfg.tOn);
  s_prefs.putFloat("fan_tfl", cfg.tFull);
  s_prefs.end();
  Serial.printf("[NVS] fan saved: en=%d tOn=%.1f tFull=%.1f\n",
                (int)cfg.enabled, cfg.tOn, cfg.tFull);
}
