#pragma once

// UI entry point: all includes, configs, callbacks, setup and rebuild.
// Must be included by the main sketch AFTER: display, model, uiManager, powerBtn, proto are defined.

#include "../config.h"
#include "driver/gpio.h"

#include "../AsciiProto.h"
#include "../model/model.h"
#include "ui_manager.h"
#include "../i18n.h"
#include "../buzzer.h"
#include "../settings_persistence.h"
#include "config/page_home_config.h"
#include "config/page_settings_config.h"
#include "config/page_loading_config.h"
#include "widgets/widget_weather.h"

// Fonts (single place to avoid redefinition)
#include "assets/fonts/RobotoBold20.h"
#include "assets/fonts/RobotoBold16.h"
#include "assets/fonts/RobotoBold12.h"
#include "assets/fonts/RobotoBold8.h"
#include "assets/icons_min.h"
#include "../weather_icons.h"

// -----------------------------------------------------------------------------
// Default page configs
// -----------------------------------------------------------------------------
static const HomePageConfig kHomePageConfig = {
  &Roboto_Bold20pt7b,
  &Roboto_Bold16pt7b,
  &Roboto_Bold20pt7b,
  &Roboto_Bold12pt7b,
  16,
  ADD_24x24,
  24, 24,
  LIGHT_OFF_24x24,
  SETTINGS,
  DAY_MODE_66x66,
  NIGHT_MODE_60x60
};
static const SettingsPageConfig kSettingsPageConfig = {
  &Roboto_Bold16pt7b,
  ARROW_BACK_24x24,
  ARROW_FORWARD_24x24,
  VOLUME_UP_24x24,
  VOLUME_OFF_24x24
};
static const LoadingPageConfig kLoadingPageConfig = {
  &Roboto_Bold20pt7b,
  &Roboto_Bold16pt7b,
  &Roboto_Bold12pt7b,
  &Roboto_Bold8pt7b,
  TIMER_UP_72x72,
  TIMER_MID_72x72,
  TIMER_COMPLETE_72x72
};

// -----------------------------------------------------------------------------
// Callbacks (use model, uiManager, powerBtn from main sketch)
// -----------------------------------------------------------------------------
static void onHomeButtonClick(void* user) {
  (void)user;
  buzzerBeep(model);
  Serial.println("[UI] Home button clicked");
}

static void* s_allOffUser = nullptr;

static void onAllOffButtonClick(void* user) {
  buzzerBeep(model);
  AsciiProto* p = static_cast<AsciiProto*>(user);
  if (p) {
    bool ok = p->sendWithAck("ALL_OFF", nullptr, 0, 1000, 2);
    Serial.printf("[UI] All off sent to Pi, ACK=%s\n", ok ? "OK" : "FAIL");
  } else {
    Serial.println("[UI] All off button clicked (no proto)");
  }
}

static void onDayNightSelect(uint8_t mode, void* user) {
  Model* m = (Model*)user;
  m->setDayMode(mode);
}

static void onSettingsButtonClick(void* user) {
  buzzerBeep(model);
  auto* mgr = static_cast<decltype(uiManager)*>(user);
  if (mgr) mgr->navigateTo(PageId::Settings);
}

// PWM duty per brightness level (0=off, 1-4=25/50/75/100%).
static const uint8_t kBrightnessDuty[5] = {0, 64, 128, 192, 255};

static void applyFrontLight(uint8_t level) {
  if (level > 4) level = 4;
  uint8_t duty = kBrightnessDuty[level];
  if (duty == 0) {
    // Detach LEDC, then drive the gate LOW directly via ESP-IDF.
    // gpio_reset_pin is intentionally NOT used: it puts the pin in INPUT/floating
    // state which lets the MOSFET gate capacitance hold charge and keep it on.
    ledcDetach(PIN_FRONT_LIGHT);
    gpio_set_direction((gpio_num_t)PIN_FRONT_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_FRONT_LIGHT, 0);
  } else {
    ledcAttach(PIN_FRONT_LIGHT, FRONT_LIGHT_FREQ_HZ, FRONT_LIGHT_RES_BITS);
    ledcWrite(PIN_FRONT_LIGHT, duty);
  }
}

static void onBrightnessChange(int delta, void* user) {
  Model* m = (Model*)user;
  int v = (int)m->settings.brightness + delta;
  if (v < 0) v = 0;
  if (v > 4) v = 4;
  m->setBrightness((uint8_t)v);
  applyFrontLight((uint8_t)v);
}

static void onSoundChange(bool enabled, void* user) {
  Model* m = (Model*)user;
  m->setSoundEnabled(enabled);
  if (enabled) buzzerBeep(*m);
}

// -----------------------------------------------------------------------------
// Setup and rebuild (templates: DisplayT deduced from uiManager)
// -----------------------------------------------------------------------------
// onSettingsBackClick is defined in the main sketch (so it can call setupUI_rebuildAndGoHome with concrete DisplayT).
template<typename DisplayT>
static void setupUI(
  UIManager<DisplayT>& uiManager,
  Model& model,
  void (*onBack)(void*),
  void* backUser,
  void* allOffUser = nullptr)
{
  s_allOffUser = allOffUser;
  loadSettings(model);
  i18n::init(model.settings.language);
  uiManager.pageHome().setWeatherIconFn(getWeatherIcon);
  applyHomeConfig(
    uiManager.pageHome(),
    kHomePageConfig,
    onHomeButtonClick, nullptr,
    onAllOffButtonClick, s_allOffUser,
    onSettingsButtonClick, &uiManager,
    onDayNightSelect, &model
  );
  applySettingsConfig(
    uiManager.pageSettings(),
    kSettingsPageConfig,
    onBrightnessChange, &model,
    onSoundChange, &model,
    onBack,
    backUser
  );
  applyLoadingConfig(uiManager.pageLoading(), kLoadingPageConfig);
  uiManager.pageShutdown().setFonts(&Roboto_Bold20pt7b, &Roboto_Bold16pt7b, &Roboto_Bold12pt7b);
  uiManager.pageShutdown().setIcon(POWER_PLUG_OFF_72x72);
  model.setLoadingActive(true);
  model.setLoadingReason(i18n::loading().reasonDefault);
  uiManager.begin(PageId::Loading);
}

template<typename DisplayT>
static void rebuildPageAfterLanguageChange(uint8_t langId, UIManager<DisplayT>& uiManager) {
  i18n::init(langId);
  applyHomeConfig(
    uiManager.pageHome(),
    kHomePageConfig,
    onHomeButtonClick, nullptr,
    onAllOffButtonClick, s_allOffUser,
    onSettingsButtonClick, &uiManager,
    onDayNightSelect, &model
  );
  uiManager.redrawCurrentPage();
}

// Called from onSettingsBackClick: save already done by caller; rebuild and navigate to Home.
template<typename DisplayT>
static void setupUI_rebuildAndGoHome(UIManager<DisplayT>& uiManager, Model& model) {
  rebuildPageAfterLanguageChange(model.settings.language, uiManager);
  uiManager.navigateTo(PageId::Home);
}
