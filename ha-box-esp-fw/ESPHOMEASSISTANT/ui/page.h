#pragma once
#include <stdint.h>
#include "../model/model.h"

template<typename DisplayT>
struct Page
{
  virtual ~Page() = default;

  // ---- Lifecycle ----
  // Called when entering this page (before drawFull)
  virtual void onEnter(Model& model) { (void)model; }
  
  // Called when exiting this page (before switching to another)
  virtual void onExit(Model& model) { (void)model; }

  // Called every frame (animations, timers, local logic, etc.)
  virtual void tick(Model& model) { (void)model; }

  // Called when a touch is detected (x, y in display coordinates). Return true if consumed.
  virtual bool onTouch(int16_t x, int16_t y) { (void)x; (void)y; return false; }

  // ---- Rendering ----
  // Full window render (on page change or "reset UI")
  virtual void drawFull(DisplayT& display, const Model& model) = 0;

  // Mark widgets as dirty based on model dirty mask
  virtual void applyDirty(uint32_t dirtyMask) = 0;

  // Partial refresh: only dirty widgets.
  // budget = max number of partial refreshes per loop.
  virtual void renderPartials(DisplayT& display, const Model& model, uint8_t budget) = 0;
};