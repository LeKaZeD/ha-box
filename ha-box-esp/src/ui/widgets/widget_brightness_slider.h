#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_draw.h"

struct BrightnessSliderWidget : public WidgetBase<BrightnessSliderWidget>
{
  static constexpr int16_t SEGMENT_COUNT = 4;
  static constexpr int16_t SLIDER_ROW_HEIGHT = 36;
  static constexpr int16_t SIDE_BTN_SIZE = 36;
  static constexpr int16_t SEGMENT_HEIGHT = 24;
  static constexpr int16_t ICON_SIZE = 24;

  static constexpr int16_t HORIZONTAL_MARGIN = 16;
  static constexpr int16_t GAP_BETWEEN_BTN_AND_SEGMENTS = 4;
  static constexpr int16_t GAP_BETWEEN_SEGMENTS = 4;
  static constexpr int16_t SEGMENT_CORNER_RADIUS = 6;
  static constexpr int16_t BTN_CORNER_RADIUS = 18;

  const GFXfont* font = nullptr;
  const uint8_t* iconBack = nullptr;
  const uint8_t* iconForward = nullptr;

  using ChangeFn = void (*)(int delta, void* user);
  ChangeFn changeCb = nullptr;
  void* changeUser = nullptr;

  explicit BrightnessSliderWidget(Rect rr) : WidgetBase<BrightnessSliderWidget>(rr) {}

  void setFont(const GFXfont* f) { font = f; }
  void setIcons(const uint8_t* back, const uint8_t* forward) { iconBack = back; iconForward = forward; }
  void setChangeCallback(ChangeFn fn, void* user) { changeCb = fn; changeUser = user; }

  bool hitTestDecrement(int16_t px, int16_t py) const
  {
    const int16_t leftBtnX = r.x + HORIZONTAL_MARGIN;
    const int16_t rowCenterY = r.y + SLIDER_ROW_HEIGHT / 2;
    const int16_t leftBtnY = rowCenterY - SIDE_BTN_SIZE / 2;
    return px >= leftBtnX && px < leftBtnX + SIDE_BTN_SIZE &&
           py >= leftBtnY && py < leftBtnY + SIDE_BTN_SIZE;
  }

  bool hitTestIncrement(int16_t px, int16_t py) const
  {
    const int16_t contentW = r.w - 2 * HORIZONTAL_MARGIN;
    const int16_t segW = (contentW - 2 * SIDE_BTN_SIZE - 2 * GAP_BETWEEN_BTN_AND_SEGMENTS - (SEGMENT_COUNT - 1) * GAP_BETWEEN_SEGMENTS) / SEGMENT_COUNT;
    const int16_t leftBtnX = r.x + HORIZONTAL_MARGIN;
    const int16_t rightBtnX = leftBtnX + SIDE_BTN_SIZE + 2 * GAP_BETWEEN_BTN_AND_SEGMENTS + SEGMENT_COUNT * segW + SEGMENT_COUNT * GAP_BETWEEN_SEGMENTS;
    const int16_t rowCenterY = r.y + SLIDER_ROW_HEIGHT / 2;
    const int16_t rightBtnY = rowCenterY - SIDE_BTN_SIZE / 2;
    return px >= rightBtnX && px < rightBtnX + SIDE_BTN_SIZE &&
           py >= rightBtnY && py < rightBtnY + SIDE_BTN_SIZE;
  }

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & DK_ST_Brightness) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    const int16_t contentW = r.w - 2 * HORIZONTAL_MARGIN;
    const int16_t segW = (contentW - 2 * SIDE_BTN_SIZE - 2 * GAP_BETWEEN_BTN_AND_SEGMENTS - (SEGMENT_COUNT - 1) * GAP_BETWEEN_SEGMENTS) / SEGMENT_COUNT;
    const int16_t rowCenterY = r.y + SLIDER_ROW_HEIGHT / 2;

    const int16_t leftBtnX = r.x + HORIZONTAL_MARGIN;
    const int16_t leftBtnY = rowCenterY - SIDE_BTN_SIZE / 2;
    display.fillRoundRect(leftBtnX, leftBtnY, SIDE_BTN_SIZE, SIDE_BTN_SIZE, BTN_CORNER_RADIUS, GxEPD_BLACK);
    if (iconBack) {
      static uint8_t iconBuf[72];
      const size_t n = ((size_t)(ICON_SIZE + 7) / 8) * (size_t)ICON_SIZE;
      if (n <= sizeof(iconBuf)) {
        const int16_t iconX = leftBtnX + (SIDE_BTN_SIZE - ICON_SIZE) / 2;
        const int16_t iconY = leftBtnY + (SIDE_BTN_SIZE - ICON_SIZE) / 2;
        drawIcon_0Black(display, iconX, iconY, iconBack, ICON_SIZE, ICON_SIZE, GxEPD_WHITE, GxEPD_BLACK, iconBuf);
      }
    }

    const int16_t segY = rowCenterY - SEGMENT_HEIGHT / 2;
    int16_t segX = leftBtnX + SIDE_BTN_SIZE + GAP_BETWEEN_BTN_AND_SEGMENTS;
    const uint8_t level = model.settings.brightness;
    for (int i = 0; i < SEGMENT_COUNT; i++) {
      if (i < (int)level)
        display.fillRoundRect(segX, segY, segW, SEGMENT_HEIGHT, SEGMENT_CORNER_RADIUS, GxEPD_BLACK);
      else
        display.drawRoundRect(segX, segY, segW, SEGMENT_HEIGHT, SEGMENT_CORNER_RADIUS, GxEPD_BLACK);
      segX += segW + GAP_BETWEEN_SEGMENTS;
    }

    const int16_t rightBtnX = segX + GAP_BETWEEN_BTN_AND_SEGMENTS;
    const int16_t rightBtnY = rowCenterY - SIDE_BTN_SIZE / 2;
    display.fillRoundRect(rightBtnX, rightBtnY, SIDE_BTN_SIZE, SIDE_BTN_SIZE, BTN_CORNER_RADIUS, GxEPD_BLACK);
    if (iconForward) {
      static uint8_t iconBuf[72];
      const size_t n = ((size_t)(ICON_SIZE + 7) / 8) * (size_t)ICON_SIZE;
      if (n <= sizeof(iconBuf)) {
        const int16_t iconX = rightBtnX + (SIDE_BTN_SIZE - ICON_SIZE) / 2;
        const int16_t iconY = rightBtnY + (SIDE_BTN_SIZE - ICON_SIZE) / 2;
        drawIcon_0Black(display, iconX, iconY, iconForward, ICON_SIZE, ICON_SIZE, GxEPD_WHITE, GxEPD_BLACK, iconBuf);
      }
    }
  }
};
