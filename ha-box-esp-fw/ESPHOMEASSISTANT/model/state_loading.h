#pragma once
#include <stdint.h>

// Loading: raison + animation
struct LoadingState
{
  static constexpr uint8_t REASON_MAXLEN = 64;

  char reason[REASON_MAXLEN] = {0}; // ex: "Waiting for Pi...", "Connecting..."
  uint8_t anim_frame = 0;            // ex: 0..3 pour animation dots
  bool active = false;
};