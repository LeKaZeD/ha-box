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

// UART0 (Serial)  → Pi communication + bootloader flashing (GPIO1=TX, GPIO3=RX)
// UART2 (Serial2) → debug output only (GPIO17=TX, GPIO16=RX)
static constexpr uint8_t DEBUG_TX = 17;
static constexpr uint8_t DEBUG_RX = 16;

// Firmware version reported via VERSION verb on boot.
static constexpr const char* FIRMWARE_VERSION = "0.1.1";

// Button
static constexpr uint8_t BTN_PIN = 34;

// Buzzer (active high)
static constexpr uint8_t PIN_BUZZER = 32;
static constexpr uint16_t BUZZER_BEEP_MS = 100;

// Pi start/stop (J2)
static constexpr uint8_t J2_PULSE_PIN = 33;

// Pi alive signal — GPIO5 (VEN pad).
// Pi GPIO17 (pin 11) driven HIGH by systemd; floats LOW on shutdown.
// ESP32 reads LOW → Pi is off → enter deep sleep.
static constexpr uint8_t PIN_PI_ALIVE = 5;

// TMP102 case temperature sensor (for fan control).
// I2C address: ADD0=GND->0x48, ADD0=V+->0x49, ADD0=SDA->0x4A, ADD0=SCL->0x4B
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

// Weather
static constexpr uint8_t WEATHER_CODE_MAX = 15;

// Touch
static constexpr uint16_t TOUCH_COOLDOWN_MS = 200;
