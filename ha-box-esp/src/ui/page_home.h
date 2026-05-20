#pragma once
#include "page.h"
#include "widgets/widget_status_icons.h"
#include "widgets/widget_weather.h"
#include "widgets/widget_temp.h"
#include "widgets/widget_clock.h"
#include "widgets/widget_button.h"
#include "widgets/widget_alerts.h"
#include "widgets/widget_day_night_switch.h"

template<typename DisplayT>
struct PageHome : public Page<DisplayT>
{
  static constexpr int16_t COMPONENT_GAP  = 5;
  static constexpr int16_t BUTTON_HEIGHT  = 44;

  // Row layout: each Y is derived from the previous row's bottom + COMPONENT_GAP.
  // Changing ROW_CLOCK_Y shifts the entire layout below it automatically.
  static constexpr int16_t ROW_CLOCK_Y   = 60;
  static constexpr int16_t ROW_CLOCK_H   = 55;
  static constexpr int16_t ROW_TEMP_Y    = ROW_CLOCK_Y   + ROW_CLOCK_H   + COMPONENT_GAP;
  static constexpr int16_t ROW_TEMP_H    = 55;
  static constexpr int16_t ROW_DAYNIGH_Y = ROW_TEMP_Y    + ROW_TEMP_H    + COMPONENT_GAP;
  static constexpr int16_t ROW_DAYNIGH_H = 76;
  static constexpr int16_t ROW_ALERTS_Y  = ROW_DAYNIGH_Y + ROW_DAYNIGH_H + COMPONENT_GAP;
  static constexpr int16_t ROW_BTN1_Y    = ROW_ALERTS_Y  + BUTTON_HEIGHT  + COMPONENT_GAP;
  static constexpr int16_t ROW_BTN2_Y    = ROW_BTN1_Y    + BUTTON_HEIGHT  + COMPONENT_GAP;

  // Column geometry for the clock/weather horizontal split
  static constexpr int16_t COL_HALF_W       = DISPLAY_W / 2;
  static constexpr int16_t COL_FULL_W       = DISPLAY_W - MARGIN * 2;
  static constexpr int16_t COL_HALF_INNER_W = COL_HALF_W - MARGIN * 2;

  // Vertical separator bar between clock and weather columns
  static constexpr int16_t SEPARATOR_X      = COL_HALF_W - 2;
  static constexpr int16_t SEPARATOR_Y      = ROW_CLOCK_Y + 5;
  static constexpr int16_t SEPARATOR_W      = 4;
  static constexpr int16_t SEPARATOR_H      = ROW_CLOCK_H - 10;
  static constexpr int16_t SEPARATOR_RADIUS = 16;

  using DayNightFn = void (*)(uint8_t mode, void* user);

  // ---- Widgets ----
  StatusIconsWidget    status   { Rect{ MARGIN, MARGIN_TOP,    COL_FULL_W,       24            } };
  ClockWidget          clock    { Rect{ MARGIN, ROW_CLOCK_Y,   COL_HALF_INNER_W, ROW_CLOCK_H   } };
  WeatherWidget        weather  { Rect{ MARGIN + COL_HALF_W,   ROW_CLOCK_Y,   COL_HALF_INNER_W, ROW_CLOCK_H   } };
  TempWidget           tempIn   { Rect{ MARGIN, ROW_TEMP_Y,    COL_FULL_W,       ROW_TEMP_H    }, TempWidget::Kind::Indoor };
  DayNightSwitchWidget dayNight { Rect{ MARGIN, ROW_DAYNIGH_Y, COL_FULL_W,       ROW_DAYNIGH_H } };
  AlertsWidget         alerts       { Rect{ MARGIN, ROW_ALERTS_Y, COL_FULL_W, BUTTON_HEIGHT } };
  ButtonWidget         buttonAllOff { Rect{ MARGIN, ROW_BTN1_Y,   COL_FULL_W, BUTTON_HEIGHT } };
  ButtonWidget         buttonSettings { Rect{ MARGIN, ROW_BTN2_Y, COL_FULL_W, BUTTON_HEIGHT } };

  DayNightFn dayNightCb_ = nullptr;
  void* dayNightUser_ = nullptr;

  // ---- Wiring options ----
  void setFonts(const GFXfont* clockFont, const GFXfont* tempFont, const GFXfont* weatherTempFont)
  {
    clock.font = clockFont;
    tempIn.font = tempFont;
    weather.font = weatherTempFont;
  }

  void setAlertsFont(const GFXfont* font) { alerts.font = font; }
  void setAlertsLang(const LangHome* l)   { alerts.lang = l; }

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

      // header separator line
      display.drawLine(0, HEADER_HEIGHT, DISPLAY_W - 1, HEADER_HEIGHT, GxEPD_BLACK);
      display.fillRoundRect(SEPARATOR_X, SEPARATOR_Y, SEPARATOR_W, SEPARATOR_H, SEPARATOR_RADIUS, GxEPD_BLACK);

      // ---- Widgets ----
      status.renderFull(display, model);
      clock.renderFull(display, model);
      weather.renderFull(display, model);
      tempIn.renderFull(display, model);
      dayNight.renderFull(display, model);
      alerts.renderFull(display, model);
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
    alerts.applyDirty(dirtyMask);
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
    if (budget && alerts.dirty)   { alerts.renderPartial(display, model);   budget--; }
    if (budget && buttonAllOff.dirty) { buttonAllOff.renderPartial(display, model); budget--; }
    if (budget && buttonSettings.dirty) { buttonSettings.renderPartial(display, model); budget--; }
  }
};