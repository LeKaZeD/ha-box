#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"

struct DayNightSwitchWidget : public WidgetBase<DayNightSwitchWidget>
{
  static constexpr int16_t CORNER_RADIUS = 16;
  static constexpr int16_t GAP = 8;
  static constexpr int16_t DAY_ICON_W = 66;
  static constexpr int16_t DAY_ICON_H = 66;
  static constexpr int16_t NIGHT_ICON_W = 60;
  static constexpr int16_t NIGHT_ICON_H = 60;

  const uint8_t* dayIconBmp = nullptr;
  const uint8_t* nightIconBmp = nullptr;

  explicit DayNightSwitchWidget(Rect rr) : WidgetBase<DayNightSwitchWidget>(rr) {}

  void setDayIcon(const uint8_t* bmp) { dayIconBmp = bmp; }
  void setNightIcon(const uint8_t* bmp) { nightIconBmp = bmp; }

  bool hitTestDay(int16_t px, int16_t py) const
  {
    const int16_t cellW = (r.w - GAP) / 2;
    return px >= r.x && px < r.x + cellW && py >= r.y && py < r.y + r.h;
  }

  bool hitTestNight(int16_t px, int16_t py) const
  {
    const int16_t cellW = (r.w - GAP) / 2;
    return px >= r.x + cellW + GAP && px < r.x + r.w && py >= r.y && py < r.y + r.h;
  }

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & DK_HM_DayMode) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    const int16_t cellW = (r.w - GAP) / 2;
    const bool daySelected = (model.home.day_mode == 0);

    static uint8_t iconBuf[594];
    const size_t dayBytes = ((size_t)(DAY_ICON_W + 7) / 8) * (size_t)DAY_ICON_H;
    const size_t nightBytes = ((size_t)(NIGHT_ICON_W + 7) / 8) * (size_t)NIGHT_ICON_H;

    Rect dayRect{ r.x, r.y, cellW, r.h };
    Rect nightRect{ r.x + cellW + GAP, r.y, cellW, r.h };

    if (daySelected) {
      display.fillRoundRect(dayRect.x, dayRect.y, dayRect.w, dayRect.h, CORNER_RADIUS, GxEPD_BLACK);
      if (dayIconBmp && dayBytes <= sizeof(iconBuf)) {
        int16_t iconX = dayRect.x + (dayRect.w - DAY_ICON_W) / 2;
        int16_t iconY = dayRect.y + (dayRect.h - DAY_ICON_H) / 2;
        drawIcon_0Black(display, iconX, iconY, dayIconBmp, DAY_ICON_W, DAY_ICON_H,
                       GxEPD_WHITE, GxEPD_BLACK, iconBuf);
      }
      display.fillRoundRect(nightRect.x, nightRect.y, nightRect.w, nightRect.h, CORNER_RADIUS, GxEPD_WHITE);
      if (nightIconBmp && nightBytes <= sizeof(iconBuf)) {
        int16_t iconX = nightRect.x + (nightRect.w - NIGHT_ICON_W) / 2;
        int16_t iconY = nightRect.y + (nightRect.h - NIGHT_ICON_H) / 2;
        drawIcon_0Black(display, iconX, iconY, nightIconBmp, NIGHT_ICON_W, NIGHT_ICON_H,
                       GxEPD_BLACK, GxEPD_WHITE, iconBuf);
      }
    } else {
      display.fillRoundRect(dayRect.x, dayRect.y, dayRect.w, dayRect.h, CORNER_RADIUS, GxEPD_WHITE);
      if (dayIconBmp && dayBytes <= sizeof(iconBuf)) {
        int16_t iconX = dayRect.x + (dayRect.w - DAY_ICON_W) / 2;
        int16_t iconY = dayRect.y + (dayRect.h - DAY_ICON_H) / 2;
        drawIcon_0Black(display, iconX, iconY, dayIconBmp, DAY_ICON_W, DAY_ICON_H,
                       GxEPD_BLACK, GxEPD_WHITE, iconBuf);
      }
      display.fillRoundRect(nightRect.x, nightRect.y, nightRect.w, nightRect.h, CORNER_RADIUS, GxEPD_BLACK);
      if (nightIconBmp && nightBytes <= sizeof(iconBuf)) {
        int16_t iconX = nightRect.x + (nightRect.w - NIGHT_ICON_W) / 2;
        int16_t iconY = nightRect.y + (nightRect.h - NIGHT_ICON_H) / 2;
        drawIcon_0Black(display, iconX, iconY, nightIconBmp, NIGHT_ICON_W, NIGHT_ICON_H,
                       GxEPD_WHITE, GxEPD_BLACK, iconBuf);
      }
    }
    display.drawRoundRect(r.x, r.y, r.w, r.h, CORNER_RADIUS, GxEPD_BLACK);
  }
};
