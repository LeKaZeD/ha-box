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
  const GFXfont* font = nullptr; // ex: grosse police pour temp
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
    // Buffer pour icône jusqu'à 48x48 (288 bytes)
    static uint8_t tmp[288];

    // 1) icône météo
    const uint8_t* bmp = nullptr;
    int iw = 0, ih = 0;

    if (iconFn)
    {
      bmp = iconFn(model.home.weather_code, iw, ih);
    }

    // Fallback: icône N/A si pas d'icône trouvée
    if (!bmp || iw == 0 || ih == 0)
    {
      bmp = WEATHER_NA_45x45;
      iw = 45;
      ih = 45;
    }

    // Icône météo agrandie (centrée dans un espace plus grand)
    // L'icône 45x45 est dessinée dans un espace plus large pour paraître plus grande
    const int16_t iconX = r.x + 5;  // Plus d'espace à gauche
    const int16_t iconY = r.y + 5;  // Plus d'espace en haut
    drawIcon_0Black(display, iconX, iconY, bmp, iw, ih, GxEPD_BLACK, GxEPD_WHITE, tmp);
    //display.drawRoundRect(iconX, iconY, iw, ih, 10, GxEPD_BLACK);

    // 2) temp extérieure (arrondie, sans décimales) + °C
    const int16_t t = model.home.temp_out_x10;
    // Arrondir : +5 pour arrondir au lieu de tronquer
    const int16_t rounded = (t >= 0) ? (t + 5) / 10 : (t - 5) / 10;

    char tempBuf[6]; // "-99" max = 4 chars + '\0'
    snprintf(tempBuf, sizeof(tempBuf), "%d", rounded);

    const int16_t textX = r.x + 50;  // Décalé pour laisser plus de place à l'icône agrandie
    const int16_t baseY = r.y + 42;  // Ajusté verticalement

    display.setFont(font);
    display.setCursor(textX, baseY);
    display.print(tempBuf);

    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(tempBuf, textX, baseY, &x1, &y1, &w, &h);
    drawDegreeSymbol(display, textX + w + 6, baseY - h + 6, 5);  // Rayon augmenté: 4 → 5

    //display.setCursor(textX + w + 10, baseY);
    //display.print("C");
  }
};