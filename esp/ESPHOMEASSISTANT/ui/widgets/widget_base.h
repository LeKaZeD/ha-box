#pragma once
#include "rect.h"

template<typename Derived>
struct WidgetBase
{
  Rect r;
  bool dirty = true;

  explicit WidgetBase(Rect rr) : r(rr) {}

  void markDirty() { dirty = true; }

  template<typename DisplayT, typename ModelT>
  void renderPartial(DisplayT& display, const ModelT& model)
  {
    if (!dirty) return;

    display.setPartialWindow(r.x, r.y, r.w, r.h);
    display.firstPage();
    do {
      display.fillRect(r.x, r.y, r.w, r.h, GxEPD_WHITE);
      static_cast<Derived*>(this)->draw(display, model);
    } while (display.nextPage());

    dirty = false;
  }

  template<typename DisplayT, typename ModelT>
  void renderFull(DisplayT& display, const ModelT& model)
  {
    static_cast<Derived*>(this)->draw(display, model);
    dirty = false;
  }
};