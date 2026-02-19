#include "epd_uc8253.h"

static inline void pinWrite(int16_t pin, int v) { digitalWrite(pin, v); }
static inline int  pinRead(int16_t pin) { return digitalRead(pin); }
static inline int clampi_(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }
static inline int align8_down_(int v) { return v & ~7; }
static inline int align8_up_(int v) { return (v + 7) & ~7; }

const unsigned char LUT_Part[216]={              
0x01, 0x0E, 0x01, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x0E, 0x01, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x8E, 0x81, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x4E, 0x41, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x0E, 0x41, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x09, 0x10, 0x3F, 0x3F, 0x00, 0x00,   
//Frame VGH VSH VSL VSHR  VCOM    
};

void EpdUc8253::ensureFastInit_()
{
  if (_fastInited) return;
  initFast_();        // ton initFast_ actuel (E0/E5/50 + etc.)
  _partInited = false;
  _fastInited = true;
}

void EpdUc8253::ensurePartInit_()
{
  if (_partInited) return;
  initPart_();
  _partInited = true;
  _fastInited = false; // on ne mélange pas les modes
}

void EpdUc8253::writePartLut_()
{
  cmd_(0x20); for (int i = 0; i < 42;  i++) data_(LUT_Part[i]);
  cmd_(0x21); for (int i = 42; i < 84;  i++) data_(LUT_Part[i]);
  cmd_(0x22); for (int i = 84; i < 126; i++) data_(LUT_Part[i]);
  cmd_(0x23); for (int i = 126;i < 168; i++) data_(LUT_Part[i]);
  cmd_(0x24); for (int i = 168;i < 210; i++) data_(LUT_Part[i]);
}

EpdUc8253::EpdUc8253(Pins pins, int16_t sck, int16_t mosi, int16_t miso)
: _pins(pins), _sck(sck), _mosi(mosi), _miso(miso)
{
}

void EpdUc8253::begin()
{
  pinMode(_pins.busy, INPUT);
  pinMode(_pins.rst, OUTPUT);
  pinMode(_pins.dc, OUTPUT);
  pinMode(_pins.cs, OUTPUT);

  pinWrite(_pins.cs, HIGH);

  SPI.begin(_sck, _miso, _mosi, _pins.cs);

  // screen assumed white after clear; keep old buffer in sync when you clear
  memset(_old, 0xFF, sizeof(_old));
}

void EpdUc8253::resetIc_()
{
  pinWrite(_pins.rst, LOW);
  delay(10);
  pinWrite(_pins.rst, HIGH);
  delay(10);
}

void EpdUc8253::waitBusyFree_()
{
  const uint32_t t0 = millis();

  // 1) si on est free, laisser une chance au contrôleur de passer busy
  while (pinRead(_pins.busy)) {
    if (millis() - t0 > 50) break; // 50ms: s'il ne passe pas busy, tant pis
    delay(1);
  }

  // 2) attendre le retour à free
  while (!pinRead(_pins.busy)) {
    if (millis() - t0 > 5000) break; // timeout de sécurité
    delay(1);
  }
}

void EpdUc8253::cmd_(uint8_t c)
{
  pinWrite(_pins.cs, LOW);
  pinWrite(_pins.dc, LOW);
  SPI.beginTransaction(_spiSettings);
  SPI.transfer(c);
  SPI.endTransaction();
  pinWrite(_pins.cs, HIGH);
}

void EpdUc8253::data_(uint8_t d)
{
  pinWrite(_pins.cs, LOW);
  pinWrite(_pins.dc, HIGH);
  SPI.beginTransaction(_spiSettings);
  SPI.transfer(d);
  SPI.endTransaction();
  pinWrite(_pins.cs, HIGH);
}

void EpdUc8253::dataBytes_(const uint8_t* p, size_t n)
{
  digitalWrite(_pins.cs, LOW);
  digitalWrite(_pins.dc, HIGH);

  SPI.beginTransaction(_spiSettings);
  while (n--) SPI.transfer(*p++);
  SPI.endTransaction();

  digitalWrite(_pins.cs, HIGH);
}

void EpdUc8253::powerOn_()
{
  cmd_(0x04);
  waitBusyFree_();
}

void EpdUc8253::powerOff_()
{
  cmd_(0x02);
  waitBusyFree_();
  _fastInited = false;   // IMPORTANT
  _partInited = false;   // IMPORTANT
}

void EpdUc8253::initFull_()
{
  resetIc_();
  powerOn_();
  cmd_(0x50);
  data_(0x97);

  _fastInited = false;
  _partInited = false;
}

void EpdUc8253::initFast_()
{
  resetIc_();
  powerOn_();

  cmd_(0xE0); data_(0x02);
  cmd_(0xE5); data_(0x32);

  cmd_(0x50); data_(0x97);
  _fastInited = true;
  _partInited = false;
}

void EpdUc8253::initPart_()
{
  resetIc_();
  powerOn_();

  cmd_(0x00); data_(0xB7); data_(0x8D);     // Panel setting: LUT from MCU
  cmd_(0x01);                               // Power setting
  data_(0x03);
  data_(LUT_Part[211]); // VGH/VGL
  data_(LUT_Part[212]); // VSH
  data_(LUT_Part[213]); // VSL
  data_(LUT_Part[214]); // VSHR

  cmd_(0x06); data_(0xD7); data_(0xD7); data_(0x2F); // Booster
  cmd_(0x30); data_(LUT_Part[210]);                  // PLL

  cmd_(0x50); data_(0xD7);                           // CDI (partial-friendly)
  cmd_(0x60); data_(0x22);                           // TCON

  cmd_(0x65); data_(0x00); data_(0x00); data_(0x00); data_(0x00); // GSST

  cmd_(0x82); data_(LUT_Part[215]);                  // VCOM
  cmd_(0xE3); data_(0x88);                           // Power saving

  writePartLut_();

  _partInited = true;
  _fastInited = false;
}

uint8_t EpdUc8253::readReg1_(uint8_t cmd)
{
  cmd_(cmd);

  // Lecture 1 byte (souvent: DC=HIGH et un dummy read)
  pinWrite(_pins.cs, LOW);
  pinWrite(_pins.dc, HIGH);

  SPI.beginTransaction(_spiSettings);
  uint8_t v = SPI.transfer(0x00);
  SPI.endTransaction();

  pinWrite(_pins.cs, HIGH);
  return v;
}

void EpdUc8253::clearFull()
{
  initFull_();

  cmd_(0x10);
  for (uint32_t i = 0; i < 12480; i++) data_(0xFF);

  cmd_(0x13);
  for (uint32_t i = 0; i < 12480; i++) data_(0xFF);

  cmd_(0x12);
  delay(10);
  waitBusyFree_();

  memset(_old, 0xFF, sizeof(_old));
  powerOff_();
}

void EpdUc8253::displayFullQuality(const uint8_t* frame416x240)
{
  initFull_(); // <= IMPORTANT : normal/full, pas fast

  cmd_(0x10);                 // OLD
  dataBytes_(_old, 12480);

  cmd_(0x13);                 // NEW
  dataBytes_(frame416x240, 12480);
  memcpy(_old, frame416x240, 12480);

  cmd_(0x12);                 // DRF
  delay(10);
  waitBusyFree_();
}

void EpdUc8253::displayFull(const uint8_t* frame416x240)
{
  initFast_();

  cmd_(0x10);
  for (uint32_t i = 0; i < 12480; i++) data_(_old[i]);

  cmd_(0x13);
  for (uint32_t i = 0; i < 12480; i++)
  {
    uint8_t v = frame416x240[i];
    data_(v);
    _old[i] = v;
  }

  cmd_(0x12);
  delay(10);
  waitBusyFree_();

  //powerOff_();
}

void EpdUc8253::displayFastNoOld(const uint8_t* frame)
{
  if (!_fastInited) { initFast_(); _fastInited = true; }

  cmd_(0x13);          // DTM2 (NEW)
  dataBytes_(frame, 480 * 416 / 8);
  cmd_(0x12);          // DRF
  delay(10);
  waitBusyFree_();
}

static inline void setPixelNative(uint8_t* dst416x240, int16_t row /*0..239*/, int16_t col /*0..415*/, bool black)
{
  // Native buffer: 416 columns => 52 bytes per row
  const int bytesPerRow = 416 / 8; // 52
  int idx = row * bytesPerRow + (col >> 3);
  int bit = 7 - (col & 7);

  if (black) dst416x240[idx] &= ~(1 << bit); // black = 0
  else       dst416x240[idx] |=  (1 << bit); // white = 1
}

void EpdUc8253::setPartialWindow_(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  // Clamp dans le repère visible
  x0 = clampi_(x0, 0, PANEL_W - 1);
  x1 = clampi_(x1, 0, PANEL_W - 1);
  y0 = clampi_(y0, 0, PANEL_H - 1);
  y1 = clampi_(y1, 0, PANEL_H - 1);

  // Align X sur 8 pixels (obligatoire)
  x0 = align8_down_(x0);
  x1 = align8_up_(x1 + 1) - 1;

  int16_t xb0 = x0 >> 3;   // 0..29
  int16_t xb1 = x1 >> 3;   // 0..29

  // UC8253 PTL: VRST/VRED sur 9 bits (0..479)
  // Ici on utilise 0..415, donc y[8] peut être 1 au-dessus de 255.
  uint16_t ry0 = (uint16_t)y0;
  uint16_t ry1 = (uint16_t)y1;

  cmd_(0x91); // PTIN

  cmd_(0x90); // PTL
  data_((uint8_t)(xb0 << 3));                 // HRST[7:3]
  data_((uint8_t)(xb1 << 3));                 // HRED[7:3]

  data_((uint8_t)((ry0 >> 8) & 0x01));        // VRST[8] en bit0
  data_((uint8_t)(ry0 & 0xFF));               // VRST[7:0]

  data_((uint8_t)((ry1 >> 8) & 0x01));        // VRED[8] en bit0
  data_((uint8_t)(ry1 & 0xFF));               // VRED[7:0]

  data_(0x01); // PT_SCAN (doc: 1 = default)
}

void EpdUc8253::writeOldWindow_(int16_t xb0, int16_t xb1, int16_t y0, int16_t y1)
{
  int len = xb1 - xb0 + 1;

  cmd_(0x10); // DTM1 = OLD
  for (int y = y0; y <= y1; y++) {
    const uint8_t* row = &_old[y * BYTES_PER_ROW + xb0];
    dataBytes_(row, len);
  }
}

void EpdUc8253::writeNewWindowFromNative_(const uint8_t* frame416x240, int16_t xb0, int16_t xb1, int16_t y0, int16_t y1)
{
  int len = xb1 - xb0 + 1;

  cmd_(0x13); // DTM2 = NEW
  for (int y = y0; y <= y1; y++) {
    const uint8_t* row = &frame416x240[y * BYTES_PER_ROW + xb0];
    dataBytes_(row, len);

    // Sync _old sur la fenêtre
    memcpy(&_old[y * BYTES_PER_ROW + xb0], row, len);
  }
}

void EpdUc8253::displayWindow(const uint8_t* frame416x240, int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (!frame416x240 || w <= 0 || h <= 0) return;

  ensurePartInit_();

  int x0 = x;
  int y0 = y;
  int x1 = x + w - 1;
  int y1 = y + h - 1;

  // Clamp avant align
  x0 = clampi_(x0, 0, PANEL_W - 1);
  x1 = clampi_(x1, 0, PANEL_W - 1);
  y0 = clampi_(y0, 0, PANEL_H - 1);
  y1 = clampi_(y1, 0, PANEL_H - 1);

  // Align X
  int ax0 = align8_down_(x0);
  int ax1 = align8_up_(x1 + 1) - 1;
  int xb0 = ax0 >> 3;
  int xb1 = ax1 >> 3;

  setPartialWindow_(ax0, y0, ax1, y1);

  writeOldWindow_(xb0, xb1, y0, y1);
  writeNewWindowFromNative_(frame416x240, xb0, xb1, y0, y1);
  
  cmd_(0x50);
  data_(0xD7);
  cmd_(0x12); // DRF
  delay(10);
  waitBusyFree_();

  cmd_(0x92); // PTOUT (Partial Out)

  _partialCount++;
  Serial.println(_partialCount);
  if (_partialCount >= 5) {
    // Reco Good Display: full refresh de temps en temps pour limiter le ghosting
    // (tu peux faire clearFull() ou un displayFull() selon ton besoin)
    _partialCount = 0;
    displayFull(_old);
    _partInited = false;
    _fastInited = false; // on repassera en fast au prochain partial
  }
  uint8_t st = readReg1_(0x71); // FLG
  Serial.printf("FLG=0x%02X\n", st);
}

void EpdUc8253::sleep()
{
  cmd_(0x02);
  waitBusyFree_();
  delay(100);
  cmd_(0x07);
  data_(0xA5);
}