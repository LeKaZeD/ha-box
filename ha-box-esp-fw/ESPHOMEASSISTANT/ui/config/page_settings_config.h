#pragma once
#include "../../i18n.h"
#include "../page_settings.h"

struct SettingsPageConfig
{
  const GFXfont* labelFont;
  const uint8_t* brightnessIconBack;
  const uint8_t* brightnessIconForward;
  const uint8_t* soundIconOn;
  const uint8_t* soundIconOff;
};

template<typename DisplayT>
void applySettingsConfig(
  PageSettings<DisplayT>& page,
  const SettingsPageConfig& cfg,
  BrightnessSliderWidget::ChangeFn onBrightness,
  void* brightnessUser,
  void (*onSound)(bool, void*),
  void* soundUser,
  void (*onBack)(void*),
  void* backUser)
{
  page.setFonts(cfg.labelFont);
  page.setBrightnessIcons(cfg.brightnessIconBack, cfg.brightnessIconForward);
  page.setBrightnessCallback(onBrightness, brightnessUser);
  page.setSoundSwitchIcons(cfg.soundIconOn, cfg.soundIconOff);
  page.setSoundCallback(onSound, soundUser);
  page.setBackCallback(onBack, backUser);
}
