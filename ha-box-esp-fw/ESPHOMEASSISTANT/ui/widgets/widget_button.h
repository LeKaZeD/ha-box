#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"

struct ButtonWidget : public WidgetBase<ButtonWidget>
{
  const GFXfont* font = nullptr;
  const char* label = "";
  int16_t cornerRadius = 8;

  const uint8_t* iconBmp = nullptr;
  int16_t iconW = 0;
  int16_t iconH = 0;

  using ClickFn = void (*)(void* user);
  ClickFn clickCb = nullptr;
  void* clickUser = nullptr;

  explicit ButtonWidget(Rect rr) : WidgetBase<ButtonWidget>(rr) {}

  void setClickCallback(ClickFn fn, void* user) { clickCb = fn; clickUser = user; }
  void setIcon(const uint8_t* bmp, int16_t w, int16_t h) { iconBmp = bmp; iconW = w; iconH = h; }

  bool hitTest(int16_t px, int16_t py) const { return r.contains(px, py); }

  void fireClick() { if (clickCb) clickCb(clickUser); }

  void applyDirty(uint32_t dirtyMask) { (void)dirtyMask; }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    (void)model;
    display.fillRoundRect(r.x, r.y, r.w, r.h, cornerRadius, GxEPD_BLACK);

    if (!font || !label) return;
    display.setFont(font);
    display.setTextColor(GxEPD_WHITE);
    int16_t x1, y1;
    uint16_t tw, th;
    display.getTextBounds(label, 0, 0, &x1, &y1, &tw, &th);
    int16_t cursorX = r.x + (r.w - (int16_t)tw) / 2;
    int16_t cursorY = r.y + (r.h - (int16_t)th) / 2 - y1;

    if (iconBmp && iconW > 0 && iconH > 0) {
      const size_t iconBytes = (size_t)iconW * (size_t)iconH / 8;
      uint8_t tmp[72];
      if (iconBytes <= sizeof(tmp)) {
        int16_t iconX = cursorX - 8 - iconW;
        int16_t iconY = r.y + (r.h - iconH) / 2;
        drawIcon_0Black(display, iconX, iconY, iconBmp, iconW, iconH, GxEPD_WHITE, GxEPD_WHITE, tmp);
      }
    }

    display.setCursor(cursorX, cursorY);
    display.print(label);
  }
};
