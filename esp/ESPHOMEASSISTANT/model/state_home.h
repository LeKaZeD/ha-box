#pragma once
#include <stdint.h>

// Home: données "dashboard"
struct HomeState
{
  // connectivité
  bool wifi_ok = false;
  bool ble_ok  = false;
  bool zigbee_ok = false;
  bool thread_ok = false;
  bool ir_ok = false;
  bool mhz433_ok = false;
  bool matter_ok = false;

  // températures en dixièmes pour éviter float : 234 => 23.4°C
  int16_t temp_out_x10 = 0;
  int16_t temp_in_x10  = 0;

  // météo (code libre, tu fais un mapping code->icone dans l'UI)
  uint8_t weather_code = 0;

  // humidité
  uint8_t humidity = 0;

  // horloge: sync from Pi (hh, mm, ss) at clockReceivedAtMs; display* = computed current time
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint32_t clockReceivedAtMs = 0;
  uint8_t displayHour = 0;
  uint8_t displayMinute = 0;
  uint8_t displaySecond = 0;

  // batterie (placeholder)
  uint8_t battery_pct = 0;

  // mode switch: 0 = day (left), 1 = night (right), 2+ reserved
  uint8_t day_mode = 0;
};