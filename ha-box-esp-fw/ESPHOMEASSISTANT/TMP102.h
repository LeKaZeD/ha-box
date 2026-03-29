#pragma once
#include <Arduino.h>
#include <Wire.h>

/**
 * Minimal driver for TI TMP102 temperature sensor (I2C).
 * Reads the 12-bit temperature register (register 0x00, 0.0625 deg C / LSB).
 * I2C address depends on ADD0 pin:
 *   GND -> 0x48, V+ -> 0x49, SDA -> 0x4A, SCL -> 0x4B
 */
class TMP102 {
public:
  explicit TMP102(uint8_t addr = 0x48);

  /**
   * Configure the I2C bus. Call before begin().
   * If sda/scl >= 0, calls Wire.begin(sda, scl, hz).
   */
  void beginWire(TwoWire& w);

  /**
   * Check device presence. Returns false if no ACK on the bus.
   * Does not block; caller decides how to handle a missing sensor.
   */
  bool begin();

  /**
   * Read current temperature. Returns false on I2C error.
   *
   * Args:
   *     tempC: output temperature in degrees Celsius.
   *
   * Returns:
   *     True on success.
   */
  bool read(float& tempC);

private:
  uint8_t   _addr;
  TwoWire*  _wire;
};
