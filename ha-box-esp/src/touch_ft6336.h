#pragma once
#include <Arduino.h>
#include <Wire.h>

class TouchFT6336
{
public:
  struct Point { uint16_t x; uint16_t y; };
  struct State { uint8_t count; Point p0; Point p1; };

  TouchFT6336(uint8_t i2cAddr = 0x38);

  void begin(int sda, int scl, int irqPin = -1);
  void setDisplayWidth(uint16_t w);   // if > 0, X is inverted: x = (w - 1) - x
  void setDisplayHeight(uint16_t h);  // if > 0, Y is inverted: y = (h - 1) - y
  bool irqActive() const;

  bool read(State& out); // returns true if count > 0

private:
  uint8_t _addr;
  int _irqPin = -1;
  uint16_t _displayWidth = 0;
  uint16_t _displayHeight = 0;

  bool readReg_(uint8_t reg, uint8_t* buf, size_t len);
};