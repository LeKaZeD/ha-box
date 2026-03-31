#pragma once

struct Model;

void loadSettings(Model& model);
void saveSettingsIfChanged(const Model& model);

// Fan configuration persisted in NVS.
// Sent by the Pi add-on on every connection; stored so the ESP32 can restore
// the correct curve after deep sleep even before the Pi reconnects.
struct FanPersistConfig {
  bool  enabled;
  float tOn;    // temperature where fan starts (°C)
  float tFull;  // temperature where fan is at full speed (°C)
};

// Returns true and populates |out| if a fan config was previously saved.
// Returns false if no config exists in NVS (first boot or after NVS clear).
bool loadFanConfig(FanPersistConfig& out);

void saveFanConfig(const FanPersistConfig& cfg);
