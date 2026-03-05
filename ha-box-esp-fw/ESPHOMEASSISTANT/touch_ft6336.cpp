#include "touch_ft6336.h"

TouchFT6336::TouchFT6336(uint8_t i2cAddr) : _addr(i2cAddr) {}

void TouchFT6336::begin(int sda, int scl, int irqPin)
{
  _irqPin = irqPin;
  if (_irqPin >= 0) pinMode(_irqPin, INPUT);

  Wire.begin(sda, scl);
  Wire.setClock(400000);
}

void TouchFT6336::setDisplayWidth(uint16_t w)
{
  _displayWidth = w;
}

void TouchFT6336::setDisplayHeight(uint16_t h)
{
  _displayHeight = h;
}

bool TouchFT6336::irqActive() const
{
  if (_irqPin < 0) return true;
  return digitalRead(_irqPin) == LOW; // common: INT low when touch
}

bool TouchFT6336::readReg_(uint8_t reg, uint8_t* buf, size_t len)
{
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  size_t got = Wire.requestFrom((int)_addr, (int)len);
  if (got != len) return false;

  for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

bool TouchFT6336::read(State& out)
{
  out = {};

  // FT6336: TD_STATUS at 0x02 (number of touch points)
  uint8_t td = 0;
  if (!readReg_(0x02, &td, 1)) return false;

  td &= 0x0F;
  if (td == 0) { out.count = 0; return false; }
  if (td > 2) td = 2;

  // Read first 2 points: registers from 0x03
  // Point1: 0x03..0x06, Point2: 0x09..0x0C
  uint8_t b[12] = {0};
  if (!readReg_(0x03, b, sizeof(b))) return false;

  auto decodePoint = [&](int base, Point& p)
  {
    uint16_t x = ((b[base + 0] & 0x0F) << 8) | b[base + 1];
    uint16_t y = ((b[base + 2] & 0x0F) << 8) | b[base + 3];
    p.x = x;
    p.y = y;
  };

  out.count = td;
  decodePoint(0, out.p0);
  if (td >= 2) decodePoint(6, out.p1);

  if (_displayWidth > 0) {
    out.p0.x = (_displayWidth - 1) - out.p0.x;
    if (td >= 2) out.p1.x = (_displayWidth - 1) - out.p1.x;
  }
  if (_displayHeight > 0) {
    out.p0.y = (_displayHeight - 1) - out.p0.y;
    if (td >= 2) out.p1.y = (_displayHeight - 1) - out.p1.y;
  }

  return true;
}