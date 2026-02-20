#pragma once
#include <Arduino.h>
#include "AsciiProto.h"

class PowerButton {
public:
  struct Config {
    int btnPin;                 // Bouton poussoir (INPUT_PULLUP)
    int j2Pin;                  // Pin qui pilote le "jump" sur J2 via transistor/opto
    bool j2ActiveHigh = true;   // true: pulse HIGH, false: pulse LOW
    uint32_t j2PulseMs = 150;   // Durée d'appui virtuel sur J2
    uint32_t j2ResetMs = 5500;  // Durée d'appui virtuel sur J2 pour reset le pi (hard stop)
    uint32_t debounceMs = 30;   // Minimal debounce
    uint32_t minClickMs = 120;  // Too short press => ignore
    uint32_t longPressMs = 5000; // Long press threshold
    uint32_t hbTimeoutMs = 5000;// Heartbeat lost => Pi considered OFF
    const char* token = nullptr;// Token expected by your bridge (optional)
  };

  enum class PiState : uint8_t { OFF, ON, SHUTDOWN_PENDING };

  PowerButton(const Config& cfg,  AsciiProto& proto);

  void begin();
  void update();

  // Call when you receive a UART line from Pi/bridge
  void onUartLine(const char* line);

  // Optional: force state if you have a reliable source
  void setPiState(PiState st);
  PiState getPiState() const;

  // Hooks for logging/UI etc.
  void setOnStateChange(void (*cb)(PiState st));

  void touchHeartbeat();

private:
  // ISR
  static void IRAM_ATTR isrThunk();
  void IRAM_ATTR onBtnEdgeISR();

  // Actions
  void handleClick();
  void handleLongPress();

  void requestShutdown();
  void pulseJ2();
  void resetJ2();
  void enterDeepSleep();

  bool isPiAliveNow(uint32_t nowMs) const;

private:
  static PowerButton* s_instance;

  Config m_cfg;
  AsciiProto& m_proto;

  volatile bool m_isrFired = false;
  volatile uint32_t m_edgeMs = 0;
  volatile uint32_t m_upDurationMs = 0;

  volatile bool m_evClick = false;
  volatile bool m_evLong  = false;

  PiState m_piState = PiState::ON;

  uint32_t m_lastHbMs = 0;
  uint32_t m_lastActionMs = 0;

  void (*m_onStateChange)(PiState) = nullptr;
};