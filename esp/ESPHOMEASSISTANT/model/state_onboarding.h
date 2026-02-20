#pragma once
#include <stdint.h>

// Onboarding: one QR code + one link (shown as text)
// QR payload: typically a URL or WiFi config string, etc.
struct OnboardingState
{
  static constexpr uint8_t QR_MAXLEN   = 128; // Reduced to save RAM
  static constexpr uint8_t LINK_MAXLEN = 64;  // Reduced to save RAM

  char qr_payload[QR_MAXLEN] = {0}; // string to encode in QR
  char link[LINK_MAXLEN]     = {0};  // string to display (short url, etc.)

  bool active = false; // set to true to enable this page
};