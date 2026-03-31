#pragma once
#include <stdint.h>
#include "../model/model.h"

// Physical display dimensions – GDEY037T03-FT21 (3.7", 240×416 px)
static constexpr int16_t DISPLAY_W     = 240;
static constexpr int16_t DISPLAY_H     = 416;

// Shared layout constants used across all pages.
static constexpr int16_t MARGIN        = 8;   // horizontal/vertical page margin (px)
static constexpr int16_t HEADER_HEIGHT = 40;  // status-bar height (px)

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