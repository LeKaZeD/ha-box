#pragma once
#include "page.h"
#include "../i18n.h"
#include "widgets/widget_status_icons.h"
#include "widgets/widget_brightness_slider.h"
#include "widgets/widget_sound_switch.h"
#include "assets/icons_draw.h"
#include "assets/icons/global/arrow_back_rounded.h"
#include "assets/icons/global/arrow_back.h"
#include "assets/icons/global/arrow_forward.h"

template<typename DisplayT>
struct PageSettings : public Page<DisplayT>
{
  static constexpr int16_t MARGIN = 8;
  static constexpr int16_t HEADER_HEIGHT = 40;
  static constexpr int16_t SCREEN_WIDTH = 240;
  static constexpr int16_t BACK_BTN_SIZE = 32;
  static constexpr int16_t BACK_BTN_RADIUS = 16;
  static constexpr int16_t TITLE_GAP = 8;
  static constexpr int16_t COMPONENT_GAP = 8;
  static constexpr int16_t LABEL_GAP = 6;

  StatusIconsWidget status { Rect{ MARGIN, 10, SCREEN_WIDTH - MARGIN*2, 24 } };
  Rect backBtnRect { MARGIN, HEADER_HEIGHT + COMPONENT_GAP, BACK_BTN_SIZE, BACK_BTN_SIZE };
  BrightnessSliderWidget brightness { Rect{ MARGIN, 0, SCREEN_WIDTH - MARGIN*2, 36 } };
  SoundSwitchWidget soundSwitch { Rect{ MARGIN, 0, SCREEN_WIDTH - MARGIN*2, SoundSwitchWidget::ROW_HEIGHT } };

  const GFXfont* labelFont = nullptr;
  using BackFn = void (*)(void* user);
  BackFn backCb_ = nullptr;
  void* backUser_ = nullptr;

  void setFonts(const GFXfont* labelFont) { this->labelFont = labelFont; brightness.setFont(labelFont); }
  void setBrightnessIcons(const uint8_t* back, const uint8_t* forward) { brightness.setIcons(back, forward); }
  void setBrightnessCallback(BrightnessSliderWidget::ChangeFn fn, void* user) { brightness.setChangeCallback(fn, user); }
  void setSoundSwitchIcons(const uint8_t* onBmp, const uint8_t* offBmp) { soundSwitch.setIcons(onBmp, offBmp); }
  void setBackCallback(BackFn fn, void* user) { backCb_ = fn; backUser_ = user; }

  bool onTouch(int16_t x, int16_t y) override
  {
    if (backBtnRect.x <= x && x < backBtnRect.x + backBtnRect.w &&
        backBtnRect.y <= y && y < backBtnRect.y + backBtnRect.h) {
      if (backCb_) backCb_(backUser_);
      return true;
    }
    if (brightness.hitTestDecrement(x, y)) {
      if (brightness.changeCb) brightness.changeCb(-1, brightness.changeUser);
      return true;
    }
    if (brightness.hitTestIncrement(x, y)) {
      if (brightness.changeCb) brightness.changeCb(1, brightness.changeUser);
      return true;
    }
    if (soundSwitch.hitTestSoundOn(x, y)) {
      if (soundCb_) soundCb_(true, soundUser_);
      return true;
    }
    if (soundSwitch.hitTestSoundOff(x, y)) {
      if (soundCb_) soundCb_(false, soundUser_);
      return true;
    }
    return false;
  }

  using SoundFn = void (*)(bool enabled, void* user);
  SoundFn soundCb_ = nullptr;
  void* soundUser_ = nullptr;
  void setSoundCallback(SoundFn fn, void* user) { soundCb_ = fn; soundUser_ = user; }

  void drawFull(DisplayT& display, const Model& model) override
  {
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH - 1, HEADER_HEIGHT, GxEPD_BLACK);

      status.renderFull(display, model);

      display.fillRoundRect(backBtnRect.x, backBtnRect.y, backBtnRect.w, backBtnRect.h, BACK_BTN_RADIUS, GxEPD_BLACK);
      {
        static uint8_t iconBuf[72];
        const int iw = 24, ih = 24;
        const size_t n = ((size_t)(iw + 7) / 8) * (size_t)ih;
        if (n <= sizeof(iconBuf)) {
          int16_t ix = backBtnRect.x + (BACK_BTN_SIZE - iw) / 2;
          int16_t iy = backBtnRect.y + (BACK_BTN_SIZE - ih) / 2;
          drawIcon_0Black(display, ix, iy, ARROW_BACK_ROUNDED, iw, ih, GxEPD_WHITE, GxEPD_BLACK, iconBuf);
        }
      }

      if (labelFont) {
        const char* title = i18n::settings().title;
        display.setFont(labelFont);
        display.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        display.getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
        const int16_t titleX = backBtnRect.x + BACK_BTN_SIZE + TITLE_GAP;
        const int16_t rowCenterY = backBtnRect.y + BACK_BTN_SIZE / 2;
        const int16_t titleBaseline = rowCenterY - y1 - (int16_t)th / 2;
        display.setCursor(titleX, titleBaseline);
        display.print(title);
      }

      const int16_t backBtnBottom = backBtnRect.y + backBtnRect.h;
      if (labelFont) {
        const char* labelBrightness = i18n::settings().labelBrightness;
        display.setFont(labelFont);
        display.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        display.getTextBounds(labelBrightness, 0, 0, &x1, &y1, &tw, &th);
        const int16_t labelTop = backBtnBottom + COMPONENT_GAP;
        const int16_t labelBaseline = labelTop - y1;
        display.setCursor(MARGIN, labelBaseline);
        display.print(labelBrightness);
        brightness.r.y = labelBaseline + y1 + (int16_t)th + LABEL_GAP;
      } else {
        brightness.r.y = backBtnBottom + COMPONENT_GAP;
      }

      brightness.r.h = BrightnessSliderWidget::SLIDER_ROW_HEIGHT;
      brightness.renderFull(display, model);

      const int16_t brightnessBottom = brightness.r.y + brightness.r.h;
      if (labelFont) {
        const char* labelSound = i18n::settings().labelSound;
        display.setFont(labelFont);
        display.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        display.getTextBounds(labelSound, 0, 0, &x1, &y1, &tw, &th);
        const int16_t soundLabelTop = brightnessBottom + COMPONENT_GAP;
        const int16_t soundLabelBaseline = soundLabelTop - y1;
        display.setCursor(MARGIN, soundLabelBaseline);
        display.print(labelSound);
        soundSwitch.r.y = soundLabelBaseline + y1 + (int16_t)th + LABEL_GAP;
      } else {
        soundSwitch.r.y = brightnessBottom + COMPONENT_GAP;
      }
      soundSwitch.r.h = SoundSwitchWidget::ROW_HEIGHT;
      soundSwitch.renderFull(display, model);
    } while (display.nextPage());
  }

  void applyDirty(uint32_t dirtyMask) override
  {
    status.applyDirty(dirtyMask);
    brightness.applyDirty(dirtyMask);
    soundSwitch.applyDirty(dirtyMask);
  }

  void renderPartials(DisplayT& display, const Model& model, uint8_t budget) override
  {
    if (!budget) return;
    if (budget && status.dirty) { status.renderPartial(display, model); budget--; }
    if (budget && brightness.dirty) { brightness.renderPartial(display, model); budget--; }
    if (budget && soundSwitch.dirty) { soundSwitch.renderPartial(display, model); budget--; }
  }
};
