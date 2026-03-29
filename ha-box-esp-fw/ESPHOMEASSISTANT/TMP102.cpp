#include "TMP102.h"

TMP102::TMP102(uint8_t addr) : _addr(addr), _wire(nullptr) {}

void TMP102::beginWire(TwoWire& w) {
  _wire = &w;
}

bool TMP102::begin() {
  if (!_wire) return false;
  // Probe: write pointer to temperature register, check for ACK.
  // Use stop condition (true) so the bus is released even if device is absent.
  _wire->beginTransmission(_addr);
  _wire->write(0x00);
  return (_wire->endTransmission(true) == 0);
}

bool TMP102::read(float& tempC) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(0x00);
  if (_wire->endTransmission(true) != 0) return false;
  if (_wire->requestFrom(_addr, (uint8_t)2) != 2) return false;

  uint8_t msb = _wire->read();
  uint8_t lsb = _wire->read();

  int16_t raw = (int16_t)((msb << 8) | lsb) >> 4;
  tempC = raw * 0.0625f;
  return true;
}
