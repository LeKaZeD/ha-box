#include "BME280.h"

// Registers
static const uint8_t REG_ID        = 0xD0;
static const uint8_t REG_RESET     = 0xE0;
static const uint8_t REG_CTRL_HUM  = 0xF2;
static const uint8_t REG_STATUS    = 0xF3;
static const uint8_t REG_CTRL_MEAS = 0xF4;
static const uint8_t REG_CONFIG    = 0xF5;
static const uint8_t REG_PRESS_MSB = 0xF7; // F7..FE

BME280Min::BME280Min(uint8_t addr) : _addr(addr), _wire(&Wire), _t_fine(0) {}

void BME280Min::beginWire(TwoWire &w, int sda, int scl, uint32_t hz) {
  _wire = &w;
#if defined(ESP32)
  if (sda >= 0 && scl >= 0) _wire->begin(sda, scl);
  else _wire->begin();
#else
  (void)sda; (void)scl;
  _wire->begin();
#endif
  _wire->setClock(hz);
}

bool BME280Min::begin() {
  uint8_t id = read8(REG_ID);
  if (id != 0x60) return false; // BME280

  // Soft reset
  write8(REG_RESET, 0xB6);
  delay(10);

  // Wait NVM copy (status bit0)
  for (int i = 0; i < 50; i++) {
    if ((read8(REG_STATUS) & 0x01) == 0) break;
    delay(2);
  }

  if (!readCalibration()) return false;

  // Humidity oversampling must be written before ctrl_meas
  // osrs_h = x1
  write8(REG_CTRL_HUM, 0x01);

  // config: standby 500ms, filter x16
  write8(REG_CONFIG, (0b100 << 5) | (0b100 << 2));

  // ctrl_meas: temp x1, press x1, normal mode
  write8(REG_CTRL_MEAS, (0b001 << 5) | (0b001 << 2) | 0b11);

  return true;
}

bool BME280Min::read(float &tempC, float &pressPa, float &humPct) {
  uint8_t d[8];
  if (!readN(REG_PRESS_MSB, d, 8)) return false;

  int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
  int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
  int32_t adc_H = ((int32_t)d[6] << 8)  | (int32_t)d[7];

  tempC   = compensateTempC(adc_T);
  pressPa = compensatePressPa(adc_P);
  humPct  = compensateHumPct(adc_H);
  return true;
}

// ---------- I2C helpers ----------
bool BME280Min::write8(uint8_t reg, uint8_t val) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(val);
  return _wire->endTransmission() == 0;
}

bool BME280Min::readN(uint8_t reg, uint8_t *buf, uint8_t len) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false;
  uint8_t n = _wire->requestFrom(_addr, len);
  if (n != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = _wire->read();
  return true;
}

uint8_t BME280Min::read8(uint8_t reg) {
  uint8_t v = 0xFF;
  readN(reg, &v, 1);
  return v;
}

// ---------- buffer helpers ----------
uint16_t BME280Min::u16(const uint8_t *b) { return (uint16_t)b[0] | ((uint16_t)b[1] << 8); }
int16_t  BME280Min::s16(const uint8_t *b) { return (int16_t)u16(b); }

// ---------- Calibration ----------
bool BME280Min::readCalibration() {
  uint8_t b1[26];
  if (!readN(0x88, b1, 26)) return false;

  _cal.dig_T1 = u16(&b1[0]);
  _cal.dig_T2 = s16(&b1[2]);
  _cal.dig_T3 = s16(&b1[4]);

  _cal.dig_P1 = u16(&b1[6]);
  _cal.dig_P2 = s16(&b1[8]);
  _cal.dig_P3 = s16(&b1[10]);
  _cal.dig_P4 = s16(&b1[12]);
  _cal.dig_P5 = s16(&b1[14]);
  _cal.dig_P6 = s16(&b1[16]);
  _cal.dig_P7 = s16(&b1[18]);
  _cal.dig_P8 = s16(&b1[20]);
  _cal.dig_P9 = s16(&b1[22]);

  _cal.dig_H1 = b1[25]; // 0xA1

  uint8_t b2[7];
  if (!readN(0xE1, b2, 7)) return false;

  _cal.dig_H2 = (int16_t)((uint16_t)b2[0] | ((uint16_t)b2[1] << 8));
  _cal.dig_H3 = b2[2];

  // packed 12-bit signed
  int16_t h4 = (int16_t)((((int16_t)b2[3]) << 4) | (b2[4] & 0x0F));
  int16_t h5 = (int16_t)((((int16_t)b2[5]) << 4) | (b2[4] >> 4));
  if (h4 & 0x0800) h4 |= 0xF000; // sign extend
  if (h5 & 0x0800) h5 |= 0xF000;

  _cal.dig_H4 = h4;
  _cal.dig_H5 = h5;
  _cal.dig_H6 = (int8_t)b2[6];

  return true;
}

// ---------- Compensation ----------
float BME280Min::compensateTempC(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t)_cal.dig_T1 << 1))) * ((int32_t)_cal.dig_T2)) >> 11;
  int32_t var2 = (((((adc_T >> 4) - ((int32_t)_cal.dig_T1)) * ((adc_T >> 4) - ((int32_t)_cal.dig_T1))) >> 12) *
                  ((int32_t)_cal.dig_T3)) >> 14;
  _t_fine = var1 + var2;
  int32_t T = (_t_fine * 5 + 128) >> 8; // 0.01°C
  return T / 100.0f;
}

float BME280Min::compensatePressPa(int32_t adc_P) {
  int64_t var1 = (int64_t)_t_fine - 128000;
  int64_t var2 = var1 * var1 * (int64_t)_cal.dig_P6;
  var2 = var2 + ((var1 * (int64_t)_cal.dig_P5) << 17);
  var2 = var2 + (((int64_t)_cal.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)_cal.dig_P3) >> 8) + ((var1 * (int64_t)_cal.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1) * (int64_t)_cal.dig_P1) >> 33;

  if (var1 == 0) return NAN;

  int64_t p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = ((int64_t)_cal.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
  var2 = ((int64_t)_cal.dig_P8 * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)_cal.dig_P7) << 4);

  return (float)p / 256.0f; // Q24.8 Pa
}

float BME280Min::compensateHumPct(int32_t adc_H) {
  int32_t v_x1_u32r;

  v_x1_u32r = _t_fine - ((int32_t)76800);

  v_x1_u32r = (((((adc_H << 14)
                  - (((int32_t)_cal.dig_H4) << 20)
                  - (((int32_t)_cal.dig_H5) * v_x1_u32r))
                 + ((int32_t)16384)) >> 15)
               * (((((((v_x1_u32r * ((int32_t)_cal.dig_H6)) >> 10)
                      * (((v_x1_u32r * ((int32_t)_cal.dig_H3)) >> 11)
                         + ((int32_t)32768))) >> 10)
                    + ((int32_t)2097152))
                   * ((int32_t)_cal.dig_H2)
                   + 8192) >> 14));

  v_x1_u32r = v_x1_u32r
              - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
                  * ((int32_t)_cal.dig_H1)) >> 4);

  if (v_x1_u32r < 0) v_x1_u32r = 0;
  if (v_x1_u32r > 419430400) v_x1_u32r = 419430400;

  return (v_x1_u32r >> 12) / 1024.0f;
}