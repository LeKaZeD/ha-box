#pragma once
#include <Arduino.h>
#include "AsciiProto.h"

class PowerButton {
public:
  struct Config {
    int btnPin;                 // Bouton poussoir — pull-up externe sur PCB (GPIO34: pas de pull-up interne)
    int j2Pin;                  // Pin qui pilote le "jump" sur J2 via transistor/opto
    bool j2ActiveHigh = true;   // true: pulse HIGH, false: pulse LOW
    uint32_t j2PulseMs = 150;   // J2 pulse duration (simulates a short button press)
    uint32_t j2ResetMs = 5500;  // J2 pulse duration for hard reset (simulates a long press)
    uint32_t debounceMs = 30;   // Minimal debounce
    uint32_t minClickMs = 120;  // Too short press => ignore
    uint32_t maxClickMs = 2000; // Too long for click (2s max) => ignore (only long press 5s+)
    uint32_t longPressMs = 5000; // Long press threshold
    uint32_t hbTimeoutMs = 5000;// Heartbeat lost => Pi considered OFF (startup, involuntary disconnect)
    uint32_t shutdownPendingTimeoutMs = 60000; // Fallback if SHUTDOWN_ACCEPTED never arrives
    uint32_t haltingTimeoutMs = 15000; // Silence after SHUTDOWN_ACCEPTED => Pi is off (fallback)
    int piAlivePin = -1;              // GPIO monitored for Pi alive signal (-1 = disabled)
    const char* token = nullptr;// Token expected by your bridge (optional)
  };

  enum class PiState : uint8_t { OFF, ON, SHUTDOWN_PENDING, HALTING };

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