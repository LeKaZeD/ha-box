#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../../lang/strings.h"
#include "../assets/icons_draw.h"
#include "../assets/icons_min.h"
#include <stdio.h>
#include <string.h>

struct AlertsWidget : public WidgetBase<AlertsWidget>
{
  const GFXfont*  font         = nullptr;
  int16_t         cornerRadius = 16;
  const LangHome* lang         = nullptr;   // pointer to active language strings

  explicit AlertsWidget(Rect rr) : WidgetBase<AlertsWidget>(rr) {}

  void applyDirty(uint32_t dirtyMask) {
    if (dirtyMask & DK_HM_Alerts) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    // White fill + black outline (inverse of ButtonWidget which does fillRoundRect black)
    display.fillRoundRect(r.x, r.y, r.w, r.h, cornerRadius, GxEPD_WHITE);
    display.drawRoundRect(r.x, r.y, r.w, r.h, cornerRadius, GxEPD_BLACK);

    if (!font || !lang) return;

    const uint8_t err  = model.home.errors_count;
    const uint8_t warn = model.home.warnings_count;

    const uint8_t* iconBmp = nullptr;
    char labelBuf[28];

    // Priority: Critical > Warning > All clear
    if (err > 0) {
      iconBmp = ERROR_CIRCLE_24x24;
      snprintf(labelBuf, sizeof(labelBuf), "%d %s", err, lang->alertCritical);
    } else if (warn > 0) {
      iconBmp = WARNING_CIRCLE_24x24;
      snprintf(labelBuf, sizeof(labelBuf), "%d %s", warn, lang->alertWarning);
    } else {
      iconBmp = CHECK_CIRCLE_24x24;
      strncpy(labelBuf, lang->alertClear, sizeof(labelBuf) - 1);
      labelBuf[sizeof(labelBuf) - 1] = '\0';
    }

    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);
    int16_t  x1, y1;
    uint16_t tw, th;
    display.getTextBounds(labelBuf, 0, 0, &x1, &y1, &tw, &th);

    // Horizontal centering: icon (24px) + gap (8px) + text
    const int16_t iconW  = 24;
    const int16_t gap    = 8;
    int16_t totalW = iconW + gap + (int16_t)tw;
    int16_t startX = r.x + (r.w - totalW) / 2;
    int16_t iconY  = r.y + (r.h - iconW) / 2;
    int16_t textY  = r.y + (r.h - (int16_t)th) / 2 - y1;

    // Draw icon: bit-0 = black on white background
    uint8_t tmp[72];
    drawIcon_0Black(display, startX, iconY, iconBmp, iconW, iconW,
                    GxEPD_BLACK, GxEPD_WHITE, tmp);

    display.setCursor(startX + iconW + gap, textY);
    display.print(labelBuf);
  }
};
