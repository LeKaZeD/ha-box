#pragma once
#include "page.h"
#include "../i18n.h"
#include "widgets/widget_loading.h"

template<typename DisplayT>
struct PageLoading : public Page<DisplayT>
{
  static constexpr int16_t BOX_SIZE = LoadingWidget::ICON_SIZE + LoadingWidget::BOX_PADDING * 2;

  LoadingWidget loading { Rect{ (DISPLAY_W - BOX_SIZE) / 2, (DISPLAY_H - BOX_SIZE) / 2, BOX_SIZE, BOX_SIZE } };

  const GFXfont* fontTitle = nullptr;
  const GFXfont* fontSubtitle = nullptr;
  const GFXfont* fontReason = nullptr;
  const GFXfont* fontWarning = nullptr;

  void setFonts(const GFXfont* titleFont, const GFXfont* subtitleFont,
                const GFXfont* reasonFont, const GFXfont* warningFont)
  {
    fontTitle = titleFont;
    fontSubtitle = subtitleFont;
    fontReason = reasonFont;
    fontWarning = warningFont;
  }

  void setTimerIcons(const uint8_t* up, const uint8_t* mid, const uint8_t* complete)
  {
    loading.setTimerIcons(up, mid, complete);
  }

  void onEnter(Model& model) override
  {
    model.setLoadingActive(true);
  }

  void onExit(Model& model) override
  {
    model.setLoadingActive(false);
  }

  void drawFull(DisplayT& display, const Model& model) override
  {
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      drawStaticContent(display, model);
      loading.renderFull(display, model);
    } while (display.nextPage());
  }

  void applyDirty(uint32_t dirtyMask) override
  {
    loading.applyDirty(dirtyMask);
  }

  void renderPartials(DisplayT& display, const Model& model, uint8_t budget) override
  {
    if (!budget) return;
    if (loading.dirty) {
      loading.renderPartial(display, model);
      budget--;
    }
  }

  void tick(Model& model) override
  {
    if (!model.loading.active) return;

    static uint32_t s_lastAnimMs = 0;
    static uint8_t s_animFrame = 0;
    uint32_t now = millis();
    if (s_lastAnimMs == 0) s_lastAnimMs = now;

    uint32_t delayMs = (s_animFrame == 2) ? 3000u : 2000u;
    if (now - s_lastAnimMs >= delayMs) {
      s_animFrame = (s_animFrame + 1) % 3;
      s_lastAnimMs = now;
      model.setLoadingAnimFrame(s_animFrame);
    }
  }

private:
  void drawStaticContent(DisplayT& display, const Model& model)
  {
    display.setTextColor(GxEPD_BLACK);

    static constexpr int16_t SUBTITLE_LINE_GAP = 6;
    const int16_t titleY = 18;
    int16_t subtitleY = 54;
    const int16_t reasonY = loading.r.y + loading.r.h + 22;
    static constexpr int16_t BOTTOM_MARGIN = 10;
    static constexpr int16_t WARNING_LINE_GAP = 4;

    const LangLoading& L = i18n::loading();
    if (fontTitle) {
      display.setFont(fontTitle);
      int16_t x1, y1;
      uint16_t tw, th;
      display.getTextBounds(L.title, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, titleY - y1);
      display.print(L.title);
      subtitleY = titleY - y1 + (int16_t)th;
    }

    if (fontSubtitle) {
      display.setFont(fontSubtitle);
      int16_t x1, y1;
      uint16_t tw, th;
      display.getTextBounds(L.subtitleLine1, 0, 0, &x1, &y1, &tw, &th);
      const int16_t subtitleLineH = (int16_t)th;
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, subtitleY - y1);
      display.print(L.subtitleLine1);
      display.getTextBounds(L.subtitleLine2, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, subtitleY + subtitleLineH + SUBTITLE_LINE_GAP - y1);
      display.print(L.subtitleLine2);
    }

    if (fontReason) {
      display.setFont(fontReason);
      int16_t x1, y1;
      uint16_t tw, th;
      const char* reason = model.loading.reason[0] ? model.loading.reason : " ";
      display.getTextBounds(reason, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, reasonY - y1);
      display.print(reason);
      if (model.loading.reason[0]) {
        static constexpr int16_t REASON_LINE_GAP = 6;
        const int16_t enCoursY = reasonY - y1 + (int16_t)th + REASON_LINE_GAP;
        display.getTextBounds(L.reasonSuffixInProgress, 0, 0, &x1, &y1, &tw, &th);
        display.setCursor((DISPLAY_W - (int16_t)tw) / 2, enCoursY);
        display.print(L.reasonSuffixInProgress);
      }
    }

    if (fontWarning) {
      display.setFont(fontWarning);
      int16_t x1, y1;
      uint16_t tw, th;
      const char* line1 = L.warningLine1;
      const char* line2 = L.warningLine2;
      const char* line3 = L.warningLine3;
      display.getTextBounds(line1, 0, 0, &x1, &y1, &tw, &th);
      const int16_t warningLineH = (int16_t)th;
      const int16_t baselineStep = warningLineH + WARNING_LINE_GAP;
      const int16_t baseline3 = DISPLAY_H - BOTTOM_MARGIN - warningLineH;
      const int16_t baseline2 = baseline3 - baselineStep;
      const int16_t baseline1 = baseline2 - baselineStep;
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, baseline1 - y1);
      display.print(line1);
      display.getTextBounds(line2, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, baseline2 - y1);
      display.print(line2);
      display.getTextBounds(line3, 0, 0, &x1, &y1, &tw, &th);
      display.setCursor((DISPLAY_W - (int16_t)tw) / 2, baseline3 - y1);
      display.print(line3);
    }
  }
};
