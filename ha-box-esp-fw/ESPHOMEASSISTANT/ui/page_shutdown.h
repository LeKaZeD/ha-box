#pragma once
#include "page.h"
#include "../i18n.h"
#include "assets/icons_draw.h"

template<typename DisplayT>
struct PageShutdown : public Page<DisplayT>
{
  static constexpr int16_t SCREEN_W  = 240;
  static constexpr int16_t SCREEN_H  = 416;
  static constexpr int16_t MARGIN    = 16;
  static constexpr int16_t ICON_SIZE = 24;

  const GFXfont* fontTitle    = nullptr;
  const GFXfont* fontSubtitle = nullptr;
  const GFXfont* fontHint     = nullptr;
  const uint8_t* icon         = nullptr;

  void setFonts(const GFXfont* title, const GFXfont* subtitle, const GFXfont* hint)
  {
    fontTitle    = title;
    fontSubtitle = subtitle;
    fontHint     = hint;
  }

  void setIcon(const uint8_t* bmp) { icon = bmp; }

  void drawFull(DisplayT& display, const Model&) override
  {
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      drawContent(display);
    } while (display.nextPage());
  }

  void applyDirty(uint32_t) override {}
  void renderPartials(DisplayT&, const Model&, uint8_t) override {}
  void tick(Model&) override {}

private:
  void drawContent(DisplayT& display)
  {
    const LangShutdown& L = i18n::shutdown();
    display.setTextColor(GxEPD_BLACK);

    int16_t cursorY = 0;

    // Title
    if (fontTitle) {
      display.setFont(fontTitle);
      int16_t x1, y1;
      uint16_t tw, th;
      display.getTextBounds(L.title, 0, 0, &x1, &y1, &tw, &th);
      cursorY = 24;
      display.setCursor((SCREEN_W - (int16_t)tw) / 2, cursorY - y1);
      display.print(L.title);
      cursorY += (int16_t)th + 8;
    }

    // Separator line below title
    display.drawLine(MARGIN, cursorY, SCREEN_W - MARGIN, cursorY, GxEPD_BLACK);
    cursorY += 16;

    // Subtitle lines (centered, below separator)
    if (fontSubtitle) {
      display.setFont(fontSubtitle);
      int16_t x1, y1;
      uint16_t tw, th;

      display.getTextBounds(L.subtitleLine1, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((SCREEN_W - (int16_t)tw) / 2, cursorY - y1);
      display.print(L.subtitleLine1);
      cursorY += (int16_t)th + 6;

      display.getTextBounds(L.subtitleLine2, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((SCREEN_W - (int16_t)tw) / 2, cursorY - y1);
      display.print(L.subtitleLine2);
    }

    // Power icon centered in the middle of the screen
    if (icon) {
      const int16_t iconX = (SCREEN_W - ICON_SIZE) / 2;
      const int16_t iconY = SCREEN_H / 2 - ICON_SIZE / 2;
      static uint8_t iconBuf[72];
      drawIcon_0Black(display, iconX, iconY, icon, ICON_SIZE, ICON_SIZE,
                      GxEPD_BLACK, GxEPD_WHITE, iconBuf);
    }

    // Hint lines at the bottom
    if (fontHint) {
      display.setFont(fontHint);
      int16_t x1, y1;
      uint16_t tw, th;

      display.getTextBounds(L.hintLine1, 0, 0, &x1, &y1, &tw, &th);
      const int16_t lineH  = (int16_t)th + 4;
      const int16_t hint2Y = SCREEN_H - MARGIN - (int16_t)th;
      const int16_t hint1Y = hint2Y - lineH;

      display.setCursor((SCREEN_W - (int16_t)tw) / 2, hint1Y - y1);
      display.print(L.hintLine1);

      display.getTextBounds(L.hintLine2, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((SCREEN_W - (int16_t)tw) / 2, hint2Y - y1);
      display.print(L.hintLine2);
    }
  }
};
