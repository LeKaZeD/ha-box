#pragma once
#include <Arduino.h>
#include <Wire.h>

class BME280Min {
public:
  explicit BME280Min(uint8_t addr = 0x76);

  // Optionnel: pour ESP32 / pins custom
  void beginWire(TwoWire &w = Wire, int sda = -1, int scl = -1, uint32_t hz = 100000);

  bool begin();  // init capteur + calibration + config
  bool read(float &tempC, float &pressPa, float &humPct);

private:
  uint8_t _addr;
  TwoWire *_wire;

  struct Calib {
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4, dig_H5;
    int8_t dig_H6;
  } _cal;

  int32_t _t_fine;

  // I2C helpers
  bool write8(uint8_t reg, uint8_t val);
  bool readN(uint8_t reg, uint8_t *buf, uint8_t len);
  uint8_t read8(uint8_t reg);

  // calib + compensation
  bool readCalibration();

  float compensateTempC(int32_t adc_T);
  float compensatePressPa(int32_t adc_P);
  float compensateHumPct(int32_t adc_H);

  static uint16_t u16(const uint8_t *b);
  static int16_t s16(const uint8_t *b);
};