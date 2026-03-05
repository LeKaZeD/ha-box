#pragma once
#include <stdint.h>

// Home: dashboard data
struct HomeState
{
  // connectivity (STATUS: core/sup stored for later; net/lan/wifi/ext drive status bar)
  bool core_ok = false;  // HA Core (stored, no UI yet)
  bool sup_ok  = false;  // Supervisor (stored, no UI yet)
  bool net_ok  = false;  // local network
  bool lan_ok  = false;  // connected via LAN
  bool wifi_ok = false;  // connected via WiFi
  bool ext_ok  = false;  // exposed to internet (web icon in status bar)
  bool ble_ok  = false;  // unused in status bar (icon kept in assets)
  bool zigbee_ok = false;
  bool thread_ok = false;
  bool ir_ok = false;
  bool mhz433_ok = false;
  bool matter_ok = false;

  // temperatures in tenths to avoid float: 234 => 23.4 C
  int16_t temp_out_x10 = 0;
  int16_t temp_in_x10  = 0;

  // weather (free-form code; UI maps code to icon)
  uint8_t weather_code = 0;

  // humidity
  uint8_t humidity = 0;

  // clock: sync from Pi (hh, mm, ss) at clockReceivedAtMs; display* = computed current time
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint32_t clockReceivedAtMs = 0;
  uint8_t displayHour = 0;
  uint8_t displayMinute = 0;
  uint8_t displaySecond = 0;

  // mode switch: 0 = day (left), 1 = night (right), 2+ reserved
  uint8_t day_mode = 0;
};