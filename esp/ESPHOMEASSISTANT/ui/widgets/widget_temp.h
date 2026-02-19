#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"
#include "../assets/icons_min.h"

struct TempWidget : public WidgetBase<TempWidget>
{
  enum class Kind : uint8_t { Indoor, Outdoor };
  Kind kind;

  // UI config
  const GFXfont* font = nullptr;  // ex: &MyFont18pt7b
  bool showLabel = true;
  const char* label = nullptr;    // "IN" / "OUT"

  explicit TempWidget(Rect rr, Kind k) : WidgetBase<TempWidget>(rr), kind(k) {}

  void applyDirty(uint32_t dirtyMask)
  {
    if (kind == Kind::Indoor)  { if (dirtyMask & (DK_HM_TempIn | DK_HM_Humidity)) dirty = true; }
    if (kind == Kind::Outdoor) { if (dirtyMask & DK_HM_TempOut) dirty = true; }
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    display.setTextColor(GxEPD_BLACK);
    
    if (kind == Kind::Indoor)
    {
      // Température intérieure avec icône home_temp_48x48 + Humidité avec icône humidity_48x48
      static uint8_t tmp[384]; // 48x48: 6 bytes/row × 48 rows = 288 bytes
      
      // Layout: deux lignes verticales
      // Ligne 1: Icône temp (48x48) + Température
      // Ligne 2: Icône humidity (48x48) + Humidité
      
      // 1) Icône température intérieure (48x48) - en haut
      const int16_t tempIconX = r.x + 2;
      const int16_t tempIconY = r.y + 2;
      drawIcon_0Black(display, tempIconX, tempIconY, home_temp_48x48, 48, 48, GxEPD_BLACK, GxEPD_WHITE, tmp);
      
      // 2) Température arrondie (sans décimales) - à droite de l'icône
      const int16_t t = model.home.temp_in_x10;
      const int16_t rounded = (t >= 0) ? (t + 5) / 10 : (t - 5) / 10;
      
      char tempBuf[6]; // "-99" max = 4 chars + '\0'
      snprintf(tempBuf, sizeof(tempBuf), "%d", rounded);
      
      const int16_t tempX = r.x + 50; // Après l'icône 48x48 + marge
      const int16_t tempY = r.y + 35; // Centré verticalement avec l'icône (2 + 24 = 26, mais on met 30 pour aligner le texte)
      
      display.setFont(font);
      display.setCursor(tempX, tempY);
      display.print(tempBuf);
      
      // Mesurer pour placer le symbole degré
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(tempBuf, tempX, tempY, &x1, &y1, &w, &h);
      
      // Symbole degré
      const int16_t degCx = tempX + w + 7;
      const int16_t degCy = tempY - h + 6;
      drawDegreeSymbol(display, degCx, degCy, 5);
      
      // "C"
      display.setCursor(tempX + w + 12, tempY);
      display.print("C");
      
      // 3) Icône humidité (48x48) - en bas
      const int16_t humIconX = r.x + 115;
      const int16_t humIconY = r.y + 2; // Après l'icône temp (2 + 48 + 2 = 52)
      drawIcon_0Black(display, humIconX, humIconY, humidity_48x48, 48, 48, GxEPD_BLACK, GxEPD_WHITE, tmp);
      
      // 4) Valeur humidité - à droite de l'icône
      char humBuf[6]; // "99%" max = 4 chars + '\0'
      snprintf(humBuf, sizeof(humBuf), "%u%%", model.home.humidity);
      
      const int16_t humX = r.x + 160; // Même alignement que la température
      const int16_t humY = r.y + 35; // Centré verticalement avec l'icône humidity (52 + 24 = 76)
      
      display.setFont(font);
      display.setCursor(humX, humY);
      display.print(humBuf);
    }
    else
    {
      // Outdoor: comportement original (sans icône pour l'instant)
      const int16_t t = model.home.temp_out_x10;
      const int16_t rounded = (t >= 0) ? (t + 5) / 10 : (t - 5) / 10;
      
      char buf[6];
      snprintf(buf, sizeof(buf), "%d", rounded);
      
      display.setFont(font);
      const int16_t baseX = r.x + 2;
      const int16_t baseY = r.y + r.h - 6;
      
      display.setCursor(baseX, baseY);
      display.print(buf);
      
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(buf, baseX, baseY, &x1, &y1, &w, &h);
      
      const int16_t degCx = baseX + w + 6;
      const int16_t degCy = baseY - h + 6;
      drawDegreeSymbol(display, degCx, degCy, 5);
      
      display.setCursor(baseX + w + 10, baseY);
      display.print("C");
    }
    
    display.drawRoundRect(r.x, r.y, r.w, r.h, 16, GxEPD_BLACK);
  }
};