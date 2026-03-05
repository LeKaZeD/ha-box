#pragma once
#include <stdint.h>

// Bits for what to redraw (can be grouped by page/widget).
enum DirtyKey : uint32_t
{
  DK_None = 0,

  // -------- Onboarding --------
  DK_OB_QRPayload = 1u << 0,   // QR code data changed
  DK_OB_Link      = 1u << 1,   // link text/url changed

  // -------- Loading --------
  DK_LD_Reason    = 1u << 2,   // loading reason string changed
  DK_LD_Anim      = 1u << 3,   // animation frame changed
  DK_LD_Active    = 1u << 4,   // loading screen active state changed (optional)

  // -------- Home --------
  DK_HM_Wifi      = 1u << 5,   // network slot (wifi or lan icon)
  DK_HM_Ext       = 1u << 6,   // ext slot (web icon)
  DK_HM_Zigbee    = 1u << 7,   // zigbee icon
  DK_HM_Thread    = 1u << 8,   // thread icon
  DK_HM_Ir        = 1u << 9,   // ir icon
  DK_HM_Mhz433    = 1u << 10,   // mhz433 icon
  DK_HM_Matter    = 1u << 11,   // matter icon
  DK_HM_Weather   = 1u << 12,   // weather code/icon
  DK_HM_TempOut   = 1u << 13,   // outdoor temp
  DK_HM_TempIn    = 1u << 14,   // indoor temp
  DK_HM_Humidity  = 1u << 15,   // humidity
  DK_HM_Clock     = 1u << 16,   // clock
  DK_HM_DayMode   = 1u << 18,   // day/night switch

  // -------- Settings --------
  DK_ST_Brightness = 1u << 19,
  DK_ST_Sound     = 1u << 20,
  DK_ST_Language  = 1u << 21,
};