#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"

struct LoadingWidget : public WidgetBase<LoadingWidget>
{
  static constexpr int16_t ICON_SIZE = 72;
  static constexpr int16_t BOX_PADDING = 16;
  static constexpr int16_t BOX_RADIUS = 16;

  const uint8_t* iconTimerUp = nullptr;
  const uint8_t* iconTimerMid = nullptr;
  const uint8_t* iconTimerComplete = nullptr;

  explicit LoadingWidget(Rect rr) : WidgetBase<LoadingWidget>(rr) {}

  void setTimerIcons(const uint8_t* up, const uint8_t* mid, const uint8_t* complete) {
    iconTimerUp = up;
    iconTimerMid = mid;
    iconTimerComplete = complete;
  }

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & (DK_LD_Anim | DK_LD_Active)) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    display.drawRoundRect(r.x, r.y, r.w, r.h, BOX_RADIUS, GxEPD_BLACK);

    const int16_t iconX = r.x + BOX_PADDING;
    const int16_t iconY = r.y + BOX_PADDING;
    display.fillRect(iconX, iconY, ICON_SIZE, ICON_SIZE, GxEPD_WHITE);

    const uint8_t* iconBmp = nullptr;
    if (iconTimerUp && iconTimerMid && iconTimerComplete) {
      switch (model.loading.anim_frame % 3) {
        case 0: iconBmp = iconTimerUp; break;
        case 1: iconBmp = iconTimerMid; break;
        default: iconBmp = iconTimerComplete; break;
      }
    }
    if (iconBmp) {
      static uint8_t iconBuf[648];
      const size_t iconBytes = (size_t)ICON_SIZE * (size_t)ICON_SIZE / 8;
      if (iconBytes <= sizeof(iconBuf)) {
        drawIcon_0Black(display, iconX, iconY, iconBmp, ICON_SIZE, ICON_SIZE,
                       GxEPD_BLACK, GxEPD_WHITE, iconBuf);
      }
    }
  }
};
