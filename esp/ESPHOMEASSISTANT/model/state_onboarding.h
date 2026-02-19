#pragma once
#include <stdint.h>

// Onboarding: un QR code + un lien (affiché en texte)
// QR payload : typiquement un URL, ou une string WiFi config, etc.
struct OnboardingState
{
  static constexpr uint8_t QR_MAXLEN   = 128; // Réduit pour économiser RAM
  static constexpr uint8_t LINK_MAXLEN = 64;  // Réduit pour économiser RAM

  char qr_payload[QR_MAXLEN] = {0}; // string à encoder dans le QR
  char link[LINK_MAXLEN]     = {0}; // string à afficher (url courte, etc.)

  bool active = false;               // si tu veux activer/désactiver cette page
};