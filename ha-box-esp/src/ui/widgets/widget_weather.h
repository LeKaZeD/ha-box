#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"
#include "../assets/icons_min.h"

// Optional: define your weather icons mapping in your code:
// const uint8_t* getWeatherIcon(uint8_t code, int& w, int& h);
using WeatherIconFn = const uint8_t* (*)(uint8_t code, int& w, int& h);

struct WeatherWidget : public WidgetBase<WeatherWidget>
{
  const GFXfont* font = nullptr;      // 20pt — positive temperatures
  const GFXfont* fontSmall = nullptr; // 16pt — negative temperatures (one extra char)
  WeatherIconFn iconFn = nullptr;

  explicit WeatherWidget(Rect rr) : WidgetBase<WeatherWidget>(rr) {}

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & (DK_HM_Weather | DK_HM_TempOut)) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    display.setTextColor(GxEPD_BLACK);
    //display.drawRoundRect(r.x, r.y, r.w, r.h, 10, GxEPD_BLACK);
    // Buffer for icon up to 48x48 (288 bytes)
    static uint8_t tmp[288];

    // 1) Weather icon
    const uint8_t* bmp = nullptr;
    int iw = 0, ih = 0;

    if (iconFn)
    {
      bmp = iconFn(model.home.weather_code, iw, ih);
    }

    // Fallback: N/A icon if none found
    if (!bmp || iw == 0 || ih == 0)
    {
      bmp = WEATHER_NA_45x45;
      iw = 45;
      ih = 45;
    }

    // Weather icon (centered in a larger area)
    const int16_t iconX = r.x + 5;
    const int16_t iconY = r.y + 5;
    drawIcon_0Black(display, iconX, iconY, bmp, iw, ih, GxEPD_BLACK, GxEPD_WHITE, tmp);
    //display.drawRoundRect(iconX, iconY, iw, ih, 10, GxEPD_BLACK);

    // 2) Outdoor temp (rounded, no decimals) + C
    const int16_t t = model.home.temp_out_x10;
    // Round: +5 to round instead of truncate
    const int16_t rounded = (t >= 0) ? (t + 5) / 10 : (t - 5) / 10;

    char tempBuf[6]; // "-99" max = 4 chars + '\0'
    snprintf(tempBuf, sizeof(tempBuf), "%d", rounded);

    const int16_t textX = r.x + 50;  // Offset to leave room for icon
    const int16_t baseY = r.y + 42;  // Vertical alignment

    const GFXfont* activeFont = (rounded < 0 && fontSmall) ? fontSmall : font;
    display.setFont(activeFont);
    display.setCursor(textX, baseY);
    display.print(tempBuf);

    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(tempBuf, textX, baseY, &x1, &y1, &w, &h);
    drawDegreeSymbol(display, textX + w + 6, baseY - h + 6, 5);  // radius bumped 4→5

    //display.setCursor(textX + w + 10, baseY);
    //display.print("C");
  }
};