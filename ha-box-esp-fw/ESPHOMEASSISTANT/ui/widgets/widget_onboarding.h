#pragma once
#include "widget_base.h"
#include "../../model/model.h"

struct OnboardingWidget : public WidgetBase<OnboardingWidget>
{
  const GFXfont* fontLink = nullptr;
  
  // Callback type template (deduced at call site)
  template<typename DisplayT>
  using DrawQrFn = void (*)(DisplayT& display, const Rect& qrRect, const char* payload);
  
  // Store as void* and cast when needed
  void* drawQrPtr = nullptr;

  // layout
  Rect qrRect;
  Rect linkRect;

  explicit OnboardingWidget(Rect rr) : WidgetBase<OnboardingWidget>(rr)
  {
    // default layout inside r
    qrRect   = Rect{ r.x + 10, r.y + 10, 120, 120 };
    linkRect = Rect{ r.x + 10, r.y + 140, r.w - 20, 30 };
  }

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & (DK_OB_QRPayload | DK_OB_Link)) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    display.setTextColor(GxEPD_BLACK);

    // QR area
    display.drawRect(qrRect.x, qrRect.y, qrRect.w, qrRect.h, GxEPD_BLACK);

    if (drawQrPtr)
    {
      auto drawQr = reinterpret_cast<DrawQrFn<DisplayT>>(drawQrPtr);
      drawQr(display, qrRect, model.onboarding.qr_payload);
    }
    else
    {
      // fallback: print payload (debug)
      display.setFont(nullptr);
      display.setCursor(qrRect.x + 4, qrRect.y + 12);
      display.print("QR:");
      display.setCursor(qrRect.x + 4, qrRect.y + 24);
      display.print(model.onboarding.qr_payload);
    }

    // Link text
    display.setFont(fontLink);
    display.setCursor(linkRect.x, linkRect.y + 22);
    display.print(model.onboarding.link);
  }
};