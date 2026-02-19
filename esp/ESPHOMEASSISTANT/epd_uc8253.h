#pragma once
#include <Arduino.h>
#include <SPI.h>

class EpdUc8253
{
public:
  static constexpr int16_t PANEL_W = 240;
  static constexpr int16_t PANEL_H = 416;
  struct Pins
  {
    int16_t busy;
    int16_t rst;
    int16_t dc;
    int16_t cs;
  };


  EpdUc8253(Pins pins, int16_t sck, int16_t mosi, int16_t miso = -1);

  void begin();
  void clearFull();                                // full refresh to white
  void displayFullQuality(const uint8_t* frame416x240);
  void displayFull(const uint8_t* frame416x240);   // expects 12480 bytes
  void displayWindow(const uint8_t* frame416x240, int16_t x, int16_t y, int16_t w, int16_t h);
  void displayFastNoOld(const uint8_t* frame);
  void sleep();

private:
  Pins _pins;
  int16_t _sck, _mosi, _miso;

  SPISettings _spiSettings{10000000, MSBFIRST, SPI_MODE0};
  void ensureFastInit_();
  void setPartialWindow_(int16_t x0, int16_t y0, int16_t x1, int16_t y1); // coords natives, inclusives
  void writeOldWindow_(int16_t xb0, int16_t xb1, int16_t y0, int16_t y1);
  void writeNewWindowFromNative_(const uint8_t* frame416x240, int16_t xb0, int16_t xb1, int16_t y0, int16_t y1);
  uint8_t readReg1_(uint8_t cmd);
  bool _partInited = false;

  void ensurePartInit_();
  void initPart_();
  void writePartLut_();
  uint8_t _partialCount = 0;
  bool _fastInited = false;
  static constexpr int BYTES_PER_ROW = PANEL_W / 8;   
  static constexpr int FRAME_BYTES   = BYTES_PER_ROW * PANEL_H; // 12480

  void dataBytes_(const uint8_t* p, size_t n);

  uint8_t _old[12480]{};      // previous frame (panel native)
  uint8_t _tmp[12480]{};      // conversion buffer for portrait -> native

  void resetIc_();
  void waitBusyFree_();

  void cmd_(uint8_t c);
  void data_(uint8_t d);

  void powerOn_();
  void powerOff_();

  void initFull_();
  void initFast_();
};