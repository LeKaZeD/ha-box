#pragma once
#include "page.h"
#include "widgets/widget_onboarding.h"

template<typename DisplayT>
struct PageOnboarding : public Page<DisplayT>
{
  OnboardingWidget ob { Rect{ 0, 0, DISPLAY_W, DISPLAY_H } };

  // Plug in your QR renderer and font
  template<typename QrFn>
  void setQrDrawer(QrFn fn) { ob.drawQrPtr = reinterpret_cast<void*>(fn); }
  void setFonts(const GFXfont* linkFont) { ob.fontLink = linkFont; }

  // You can also change the internal layout
  void setLayout(const Rect& qrRect, const Rect& linkRect)
  {
    ob.qrRect = qrRect;
    ob.linkRect = linkRect;
  }

  void drawFull(DisplayT& display, const Model& model) override
  {
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      display.setTextColor(GxEPD_BLACK);
      display.setFont(nullptr);
      display.setCursor(14, 24);
      display.print("Onboarding");

      ob.renderFull(display, model);
    } while (display.nextPage());
  }

  void applyDirty(uint32_t dirtyMask) override
  {
    ob.applyDirty(dirtyMask);
  }

  void renderPartials(DisplayT& display, const Model& model, uint8_t budget) override
  {
    if (!budget) return;
    if (ob.dirty) {
      ob.renderPartial(display, model);
      budget--;
    }
  }
};