#pragma once
#include "../../i18n.h"
#include "../page_home.h"

struct HomePageConfig
{
  const GFXfont* clockFont;
  const GFXfont* tempFont;
  const GFXfont* weatherFont;
  const GFXfont* buttonFont;
  int16_t buttonCornerRadius;
  const uint8_t* buttonIcon;
  int16_t buttonIconW;
  int16_t buttonIconH;
  const uint8_t* buttonAllOffIcon;
  const uint8_t* buttonSettingsIcon;
  const uint8_t* dayIcon;
  const uint8_t* nightIcon;
};

template<typename DisplayT>
void applyHomeConfig(
  PageHome<DisplayT>& page,
  const HomePageConfig& cfg,
  void (*onButton)(void*),
  void* buttonUser,
  void (*onAllOff)(void*),
  void* allOffUser,
  void (*onSettings)(void*),
  void* settingsUser,
  void (*onDayNight)(uint8_t, void*),
  void* dayNightUser)
{
  const LangHome& L = i18n::home();
  page.setFonts(cfg.clockFont, cfg.tempFont, cfg.weatherFont);
  page.setButtonFont(cfg.buttonFont);
  page.setButtonLabel(L.buttonAdd);
  page.setButtonCornerRadius(cfg.buttonCornerRadius);
  page.setButtonIcon(cfg.buttonIcon, cfg.buttonIconW, cfg.buttonIconH);
  page.setButtonClickCallback(onButton, buttonUser);

  page.setButtonAllOffFont(cfg.buttonFont);
  page.setButtonAllOffLabel(L.buttonAllOff);
  page.setButtonAllOffCornerRadius(cfg.buttonCornerRadius);
  page.setButtonAllOffIcon(cfg.buttonAllOffIcon, cfg.buttonIconW, cfg.buttonIconH);
  page.setButtonAllOffClickCallback(onAllOff, allOffUser);

  page.setButtonSettingsFont(cfg.buttonFont);
  page.setButtonSettingsLabel(L.buttonSettings);
  page.setButtonSettingsCornerRadius(cfg.buttonCornerRadius);
  page.setButtonSettingsIcon(cfg.buttonSettingsIcon, cfg.buttonIconW, cfg.buttonIconH);
  page.setButtonSettingsClickCallback(onSettings, settingsUser);

  page.setDayNightIcons(cfg.dayIcon, cfg.nightIcon);
  page.setDayNightCallback(onDayNight, dayNightUser);
}
