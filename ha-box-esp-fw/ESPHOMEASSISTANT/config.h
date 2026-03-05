#pragma once

#include <stdint.h>

// Display (e-paper)
static constexpr uint8_t PIN_BUSY = 13;
static constexpr uint8_t PIN_RST  = 12;
static constexpr uint8_t PIN_DC   = 14;
static constexpr uint8_t PIN_CS   = 27;
static constexpr uint8_t PIN_SCK  = 18;
static constexpr uint8_t PIN_MOSI = 23;

// Touch
static constexpr uint8_t PIN_TOUCH_IRQ = 36;
static constexpr uint8_t PIN_I2C_SDA   = 21;
static constexpr uint8_t PIN_I2C_SCL   = 22;

// UART to Pi
static constexpr uint8_t UART2_TX = 17;
static constexpr uint8_t UART2_RX = 16;

// Button
static constexpr uint8_t BTN_PIN = 33;

// Buzzer (active high)
static constexpr uint8_t PIN_BUZZER = 19;
static constexpr uint16_t BUZZER_BEEP_MS = 100;

// Pi start/stop (J2)
static constexpr uint8_t J2_PULSE_PIN = 26;

// Fan PWM
static constexpr uint8_t PWM_PIN = 25;
static constexpr uint16_t PWM_FREQ_HZ = 25000;
static constexpr uint8_t PWM_RES_BITS = 8;

// Timing
static constexpr uint16_t SENS_PERIOD_MS = 30000;
static constexpr uint16_t BME_PERIOD_MS   = 5000;
