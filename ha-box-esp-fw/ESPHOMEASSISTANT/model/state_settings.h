#pragma once
#include <stdint.h>

struct SettingsState
{
  uint8_t brightness = 2;
  bool sound_enabled = true;
  uint8_t language = 0;  /* 0 = FR, 1 = EN */
};
