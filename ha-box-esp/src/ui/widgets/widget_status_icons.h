#pragma once
#include "widget_base.h"
#include "../../model/model.h"
#include "../assets/icons_min.h"
#include "../assets/icons_draw.h"

struct StatusIconsWidget : public WidgetBase<StatusIconsWidget>
{
  // Constantes de layout (pas besoin de RAM)
  static constexpr int16_t ICON_W = 24;
  static constexpr int16_t ICON_H = 24;
  static constexpr int16_t WIFI_X = 0;
  static constexpr int16_t BLE_X  = 32;
  static constexpr int16_t ZIGBEE_X = 64;
  static constexpr int16_t THREAD_X = 96;
  static constexpr int16_t MATTER_X = 128;
  static constexpr int16_t IR_X = 160;
  static constexpr int16_t MHZ433_X = 192;

  static constexpr uint8_t WHITE_24x24[72] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  };

  explicit StatusIconsWidget(Rect rr) : WidgetBase<StatusIconsWidget>(rr) {}

  void applyDirty(uint32_t dirtyMask)
  {
    if (dirtyMask & (DK_HM_Wifi | DK_HM_Ext | DK_HM_Zigbee | DK_HM_Thread | DK_HM_Ir | DK_HM_Mhz433 | DK_HM_Matter)) dirty = true;
  }

  template<typename DisplayT>
  void draw(DisplayT& display, const Model& model)
  {
    static uint8_t tmp[72]; // 24x24/8 = 72 bytes exactement

    const int16_t xw = r.x + WIFI_X;
    const int16_t yw = r.y;
    const int16_t xb = r.x + BLE_X;
    const int16_t yb = r.y;
    const int16_t xz = r.x + ZIGBEE_X;
    const int16_t yz = r.y;
    const int16_t xthread = r.x + THREAD_X;
    const int16_t ythread = r.y;
    const int16_t xmatter = r.x + MATTER_X;
    const int16_t ymatter = r.y;
    const int16_t xir = r.x + IR_X;
    const int16_t yir = r.y;
    const int16_t xmhz433 = r.x + MHZ433_X;
    const int16_t ymhz433 = r.y;

    // Slot 1: WiFi icon if wifi, else LAN icon if lan, else nothing (no "wifi stop")
    const uint8_t* netBmp = model.home.wifi_ok ? WIFI_DATA_24x24 : (model.home.lan_ok ? LAN_24x24 : WHITE_24x24);
    // Slot 2: web icon if ext, else nothing (replaces BLE)
    const uint8_t* extBmp = model.home.ext_ok ? WEB_24x24 : WHITE_24x24;
    const uint8_t* zigbeeBmp = model.home.zigbee_ok ? ZIGBEE_DATA_24x24 : WHITE_24x24;
    const uint8_t* threadBmp = model.home.thread_ok ? THREAD_DATA_24x24 : WHITE_24x24;
    const uint8_t* irBmp = model.home.ir_ok ? INFRARED_DATA_24x24 : WHITE_24x24;
    const uint8_t* mhz433Bmp = model.home.mhz433_ok ? MHZ433_ON_24x24 : MHZ433_OFF_24x24;
    const uint8_t* matterBmp = model.home.matter_ok ? MATTER_DATA_24x24 : WHITE_24x24;

    drawIcon_0Black(display, xw, yw, netBmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
    drawIcon_0Black(display, xb, yb, extBmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
    drawIcon_0Black(display, xz, yz, zigbeeBmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
    drawIcon_0Black(display, xthread, ythread, threadBmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
    drawIcon_0Black(display, xmatter, ymatter, matterBmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
    drawIcon_0Black(display, xir, yir, irBmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
    drawIcon_0Black(display, xmhz433, ymhz433, mhz433Bmp, ICON_W, ICON_H, GxEPD_BLACK, GxEPD_WHITE, tmp);
  }
};