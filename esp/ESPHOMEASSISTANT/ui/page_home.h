#pragma once
#include "page.h"
#include "widgets/widget_status_icons.h"
#include "widgets/widget_weather.h"
#include "widgets/widget_temp.h"
#include "widgets/widget_clock.h"
#include "widgets/widget_button.h"
#include "widgets/widget_day_night_switch.h"

template<typename DisplayT>
struct PageHome : public Page<DisplayT>
{
  // Layout constants (économie de RAM via constexpr)
  static constexpr int16_t MARGIN = 8;
  static constexpr int16_t HEADER_HEIGHT = 40;
  static constexpr int16_t SCREEN_WIDTH = 240;
  static constexpr int16_t COMPONENT_GAP = 5;
  static constexpr int16_t BUTTON_HEIGHT = 44;

  using DayNightFn = void (*)(uint8_t mode, void* user);

  // ---- Widgets ----
  StatusIconsWidget status { Rect{ MARGIN, 10, SCREEN_WIDTH - MARGIN*2, 24 } };
  ClockWidget       clock  { Rect{ MARGIN, 60, SCREEN_WIDTH - MARGIN*2 - (SCREEN_WIDTH/2), 55 } };
  WeatherWidget     weather{ Rect{ MARGIN + SCREEN_WIDTH - (SCREEN_WIDTH/2), 60, SCREEN_WIDTH - MARGIN*2 - (SCREEN_WIDTH/2), 55 } };
  TempWidget           tempIn   { Rect{ MARGIN, 120, SCREEN_WIDTH - MARGIN*2, 55 }, TempWidget::Kind::Indoor };
  DayNightSwitchWidget dayNight { Rect{ MARGIN, 180, SCREEN_WIDTH - MARGIN*2, 76 } };
  ButtonWidget         button       { Rect{ MARGIN, 180 + 76 + COMPONENT_GAP, SCREEN_WIDTH - MARGIN*2, BUTTON_HEIGHT } };
  ButtonWidget         buttonAllOff { Rect{ MARGIN, 180 + 76 + COMPONENT_GAP + BUTTON_HEIGHT + COMPONENT_GAP, SCREEN_WIDTH - MARGIN*2, BUTTON_HEIGHT } };
  ButtonWidget         buttonSettings { Rect{ MARGIN, 180 + 76 + COMPONENT_GAP + (BUTTON_HEIGHT + COMPONENT_GAP) * 2, SCREEN_WIDTH - MARGIN*2, BUTTON_HEIGHT } };

  DayNightFn dayNightCb_ = nullptr;
  void* dayNightUser_ = nullptr;

  // ---- Wiring options ----
  void setFonts(const GFXfont* clockFont, const GFXfont* tempFont, const GFXfont* weatherTempFont)
  {
    clock.font = clockFont;
    tempIn.font = tempFont;
    weather.font = weatherTempFont;
  }

  void setButtonFont(const GFXfont* font) { button.font = font; }
  void setButtonLabel(const char* label) { button.label = label; }
  void setButtonCornerRadius(int16_t radius) { button.cornerRadius = radius; }
  void setButtonIcon(const uint8_t* bmp, int16_t w, int16_t h) { button.setIcon(bmp, w, h); }
  void setButtonClickCallback(ButtonWidget::ClickFn fn, void* user) { button.setClickCallback(fn, user); }

  void setButtonAllOffFont(const GFXfont* font) { buttonAllOff.font = font; }
  void setButtonAllOffLabel(const char* label) { buttonAllOff.label = label; }
  void setButtonAllOffCornerRadius(int16_t radius) { buttonAllOff.cornerRadius = radius; }
  void setButtonAllOffIcon(const uint8_t* bmp, int16_t w, int16_t h) { buttonAllOff.setIcon(bmp, w, h); }
  void setButtonAllOffClickCallback(ButtonWidget::ClickFn fn, void* user) { buttonAllOff.setClickCallback(fn, user); }

  void setButtonSettingsFont(const GFXfont* font) { buttonSettings.font = font; }
  void setButtonSettingsLabel(const char* label) { buttonSettings.label = label; }
  void setButtonSettingsCornerRadius(int16_t radius) { buttonSettings.cornerRadius = radius; }
  void setButtonSettingsIcon(const uint8_t* bmp, int16_t w, int16_t h) { buttonSettings.setIcon(bmp, w, h); }
  void setButtonSettingsClickCallback(ButtonWidget::ClickFn fn, void* user) { buttonSettings.setClickCallback(fn, user); }

  void setDayNightIcons(const uint8_t* dayBmp, const uint8_t* nightBmp) {
    dayNight.setDayIcon(dayBmp);
    dayNight.setNightIcon(nightBmp);
  }
  void setDayNightCallback(DayNightFn fn, void* user) { dayNightCb_ = fn; dayNightUser_ = user; }

  bool onTouch(int16_t x, int16_t y) override
  {
    if (dayNight.hitTestDay(x, y)) {
      if (dayNightCb_) dayNightCb_(0, dayNightUser_);
      return true;
    }
    if (dayNight.hitTestNight(x, y)) {
      if (dayNightCb_) dayNightCb_(1, dayNightUser_);
      return true;
    }
    if (button.hitTest(x, y)) { button.fireClick(); return true; }
    if (buttonAllOff.hitTest(x, y)) { buttonAllOff.fireClick(); return true; }
    if (buttonSettings.hitTest(x, y)) { buttonSettings.fireClick(); return true; }
    return false;
  }

  void setWeatherIconFn(WeatherIconFn fn)
  {
    weather.iconFn = fn;
  }

  void drawFull(DisplayT& display, const Model& model) override
  {
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      // ligne de séparation header
      display.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH - 1, HEADER_HEIGHT, GxEPD_BLACK);
      display.fillRoundRect((SCREEN_WIDTH/2)-2, 65, 4, 45, 16, GxEPD_BLACK);

      // ---- Widgets ----
      status.renderFull(display, model);
      clock.renderFull(display, model);
      weather.renderFull(display, model);
      tempIn.renderFull(display, model);
      dayNight.renderFull(display, model);
      button.renderFull(display, model);
      buttonAllOff.renderFull(display, model);
      buttonSettings.renderFull(display, model);

    } while (display.nextPage());
  }

  void applyDirty(uint32_t dirtyMask) override
  {
    status.applyDirty(dirtyMask);
    clock.applyDirty(dirtyMask);
    weather.applyDirty(dirtyMask);
    tempIn.applyDirty(dirtyMask);
    dayNight.applyDirty(dirtyMask);
    button.applyDirty(dirtyMask);
    buttonAllOff.applyDirty(dirtyMask);
    buttonSettings.applyDirty(dirtyMask);
  }

  void renderPartials(DisplayT& display, const Model& model, uint8_t budget) override
  {
    if (!budget) return;

    if (budget && status.dirty) { status.renderPartial(display, model); budget--; }
    if (budget && clock.dirty)  { clock.renderPartial(display, model);  budget--; }
    if (budget && weather.dirty){ weather.renderPartial(display, model);budget--; }
    if (budget && tempIn.dirty) { tempIn.renderPartial(display, model); budget--; }
    if (budget && dayNight.dirty) { dayNight.renderPartial(display, model); budget--; }
    if (budget && button.dirty) { button.renderPartial(display, model); budget--; }
    if (budget && buttonAllOff.dirty) { buttonAllOff.renderPartial(display, model); budget--; }
    if (budget && buttonSettings.dirty) { buttonSettings.renderPartial(display, model); budget--; }
  }
};