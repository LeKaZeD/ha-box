#pragma once
#include <Arduino.h>

template<typename DisplayT, size_t TMPBUF_SIZE>
static inline void drawIcon_0Black(
  DisplayT& display,
  int x, int y,
  const uint8_t* bmp,
  int w, int h,
  uint16_t color_black,
  uint16_t color_white,
  uint8_t (&tmp)[TMPBUF_SIZE]
)
{
  const size_t bpr = (size_t)(w + 7) / 8;
  const size_t n = bpr * (size_t)h;
  if (n > TMPBUF_SIZE) return;

  for (size_t i = 0; i < n; i++) {
    uint8_t b = pgm_read_byte(&bmp[i]);
    tmp[i] = ~b;
  }

  display.drawBitmap(x, y, tmp, w, h, color_black);
}

template<typename DisplayT>
static inline void drawDegreeSymbol(DisplayT& display, int16_t cx, int16_t cy, int16_t r)
{
  // Dessiner un cercle rempli pour un symbole degré plus épais et visible
  display.fillCircle(cx, cy, r, GxEPD_BLACK);
  display.fillCircle(cx, cy, r-3, GxEPD_WHITE);
}