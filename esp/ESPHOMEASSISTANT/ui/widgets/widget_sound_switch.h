#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"

struct SoundSwitchWidget : public WidgetBase<SoundSwitchWidget>
{
  static constexpr int16_t CELL_CORNER_RADIUS = 16;
  static constexpr int16_t GAP_BETWEEN_CELLS = 8;
  static constexpr int16_t ICON_SIZE = 24;
  static constexpr int16_t ROW_HEIGHT = 44;

  const uint8_t* iconSoundOn = nullptr;
  const uint8_t* iconSoundOff = nullptr;

  explicit SoundSwitchWidget(Rect rr) : WidgetBase<SoundSwitchWidget>(rr) {}

  void setIcons(const uint8_t* onBmp, const uint8_t* offBmp) {
    iconSoundOn = onBmp;
    iconSoundOff = offBmp;
  }

  bool hitTestSoundOn(int16_t px, int16_t py) const
  {
    const int16_t cellW = (r.w - GAP_BETWEEN_CELLS) / 2;
    return px >= r.x && px < r.x + cellW && py >= r.y && py < r.y + r.h;
  }

  bool hitTestSoundOff(int16_t px, int16_t py) const
  {
    const int16_t cellW = (r.w - GAP_BETWEEN_CELLS) / 2;
    return px >= r.x + cellW + GAP_BETWEEN_CELLS && px < r.x + r.w && py >= r.y && py < r.y + r.h;
  }

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & DK_ST_Sound) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    const int16_t cellW = (r.w - GAP_BETWEEN_CELLS) / 2;
    const bool soundOn = model.settings.sound_enabled;

    static uint8_t iconBuf[72];
    const size_t iconBytes = ((size_t)(ICON_SIZE + 7) / 8) * (size_t)ICON_SIZE;

    Rect leftRect{ r.x, r.y, cellW, r.h };
    Rect rightRect{ r.x + cellW + GAP_BETWEEN_CELLS, r.y, cellW, r.h };

    const int16_t iconXLeft = leftRect.x + (cellW - ICON_SIZE) / 2;
    const int16_t iconYLeft = leftRect.y + (r.h - ICON_SIZE) / 2;
    const int16_t iconXRight = rightRect.x + (cellW - ICON_SIZE) / 2;
    const int16_t iconYRight = rightRect.y + (r.h - ICON_SIZE) / 2;

    if (soundOn) {
      display.fillRoundRect(leftRect.x, leftRect.y, leftRect.w, leftRect.h, CELL_CORNER_RADIUS, GxEPD_BLACK);
      if (iconSoundOn && iconBytes <= sizeof(iconBuf)) {
        drawIcon_0Black(display, iconXLeft, iconYLeft, iconSoundOn, ICON_SIZE, ICON_SIZE, GxEPD_WHITE, GxEPD_BLACK, iconBuf);
      }
      display.fillRoundRect(rightRect.x, rightRect.y, rightRect.w, rightRect.h, CELL_CORNER_RADIUS, GxEPD_WHITE);
      if (iconSoundOff && iconBytes <= sizeof(iconBuf)) {
        drawIcon_0Black(display, iconXRight, iconYRight, iconSoundOff, ICON_SIZE, ICON_SIZE, GxEPD_BLACK, GxEPD_WHITE, iconBuf);
      }
    } else {
      display.fillRoundRect(leftRect.x, leftRect.y, leftRect.w, leftRect.h, CELL_CORNER_RADIUS, GxEPD_WHITE);
      if (iconSoundOn && iconBytes <= sizeof(iconBuf)) {
        drawIcon_0Black(display, iconXLeft, iconYLeft, iconSoundOn, ICON_SIZE, ICON_SIZE, GxEPD_BLACK, GxEPD_WHITE, iconBuf);
      }
      display.fillRoundRect(rightRect.x, rightRect.y, rightRect.w, rightRect.h, CELL_CORNER_RADIUS, GxEPD_BLACK);
      if (iconSoundOff && iconBytes <= sizeof(iconBuf)) {
        drawIcon_0Black(display, iconXRight, iconYRight, iconSoundOff, ICON_SIZE, ICON_SIZE, GxEPD_WHITE, GxEPD_BLACK, iconBuf);
      }
    }
    display.drawRoundRect(r.x, r.y, r.w, r.h, CELL_CORNER_RADIUS, GxEPD_BLACK);
  }
};
