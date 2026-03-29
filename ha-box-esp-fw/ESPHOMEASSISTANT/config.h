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

// Front-light
static constexpr uint8_t PIN_FRONT_LIGHT = 19;

// UART to Pi
static constexpr uint8_t UART2_TX = 17;
static constexpr uint8_t UART2_RX = 16;

// Button
static constexpr uint8_t BTN_PIN = 34;

// Buzzer (active high)
static constexpr uint8_t PIN_BUZZER = 32;
static constexpr uint16_t BUZZER_BEEP_MS = 100;

// Pi start/stop (J2)
static constexpr uint8_t J2_PULSE_PIN = 33;

// TMP102 case temperature sensor (for fan control)
// Set to 0 to disable TMP102 entirely (fan falls back to BME280 or safe mode).
// I2C address: ADD0=GND->0x48, ADD0=V+->0x49, ADD0=SDA->0x4A, ADD0=SCL->0x4B
#define ENABLE_TMP102 1
static constexpr uint8_t TMP102_ADDR = 0x48;

// Front-light PWM (AO3400A gate on PIN_FRONT_LIGHT)
// Recommended: 1-10 kHz to avoid visible flicker; 8-bit resolution (0-255).
static constexpr uint16_t FRONT_LIGHT_FREQ_HZ  = 5000;
static constexpr uint8_t  FRONT_LIGHT_RES_BITS = 8;

// Fan PWM
static constexpr uint8_t PWM_PIN = 25;
static constexpr uint8_t PWM_PIN_FAN_SPEED = 26;
static constexpr uint16_t PWM_FREQ_HZ = 25000;
static constexpr uint8_t PWM_RES_BITS = 8;

// Timing
static constexpr uint16_t SENS_PERIOD_MS = 30000;
static constexpr uint16_t BME_PERIOD_MS   = 5000;
