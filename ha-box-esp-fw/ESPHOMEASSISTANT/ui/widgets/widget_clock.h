#pragma once
#include "widget_base.h"
#include "../../model/model.h"

struct ClockWidget : public WidgetBase<ClockWidget>
{
  const GFXfont* font = nullptr;

  explicit ClockWidget(Rect rr) : WidgetBase<ClockWidget>(rr) {}

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & DK_HM_Clock) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    char buf[6]; // "HH:MM\0" = 6 bytes
    snprintf(buf, sizeof(buf), "%02u:%02u", model.home.displayHour, model.home.displayMinute);
    
    display.setTextColor(GxEPD_BLACK);
    display.setFont(font);

    // Position du texte
    const int16_t cursorX = r.x + 2;
    const int16_t cursorY = r.y + r.h - 14;
    
    // Mesurer la largeur maximale possible du texte (pour "99:99" au cas où)
    int16_t x1, y1;
    uint16_t maxTextW, textH;
    display.getTextBounds("99:99", cursorX, cursorY, &x1, &y1, &maxTextW, &textH);
    
    // Effacer toute la zone du rectangle (important pour partial refresh)
    // On efface un peu plus large pour être sûr de tout effacer
    const int16_t clearX = r.x;
    const int16_t clearY = r.y;
    const int16_t clearW = r.w;
    const int16_t clearH = r.h;
    display.fillRect(clearX, clearY, clearW, clearH, GxEPD_WHITE);
    
    // Dessiner le texte
    display.setCursor(cursorX, cursorY);
    display.print(buf);
    //display.drawRoundRect(r.x, r.y, r.w, r.h, 10, GxEPD_BLACK);
  }
};