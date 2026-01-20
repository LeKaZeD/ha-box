# Hardware - HA Box

This document lists all hardware components used in the HA Box project with their detailed specifications.

## Main Components

### 1. GDEY037T03-FT21 E-Paper Display

**Manufacturer**: GooDisplay (Dalian Good Display Co., Ltd.)  
**Model**: GDEY037T03-FT21  
**Type**: E-Paper (Electrophoretic Display) with integrated front-light and touch

#### Specifications

| Parameter | Value |
|-----------|-------|
| Size | 3.7" |
| Resolution | 240×416 pixels |
| DPI | 130 |
| Active area | 47.04×81.54 mm |
| Pixel pitch | 0.196×0.196 mm |
| Controller | UC8253 |
| Interface | SPI 4-wire or 3-wire |
| Front-light | 9 LEDs, 2.8V (typical) |
| Touch | FT6336U (I2C, 3.0V) |
| Operating temperature | -25°C to 70°C |
| Standby consumption | 34µA |
| Deep sleep consumption | 1.1µA |

#### E-Paper Characteristics

- **Bi-stable**: Image remains displayed without power
- **Full refresh**: ~2-3 seconds
- **Partial refresh**: ~1 second (if supported)
- **High contrast**: Excellent in ambient light
- **Ultra-low consumption**: Ideal for embedded applications
- **1-bit**: Black/white display only

#### Connection Pins

| Pin | Name | Function | Notes |
|-----|------|----------|-------|
| SPI MOSI | Data | SPI data | GPIO 10 |
| SPI SCLK | Clock | SPI clock | GPIO 11 |
| SPI CE0 | CS | Chip Select | GPIO 8 |
| DC | Data/Command | Selection | GPIO (TBD) |
| Reset | Reset | Controller reset | GPIO (TBD) |
| BUSY | Status | Controller state | GPIO (TBD, read) |
| TSCL | I2C Clock | Touch I2C clock | GPIO 3 |
| TSDA | I2C Data | Touch I2C data | GPIO 2 |
| Front-light | PWM | MOSFET front-light control | GPIO (TBD, PWM) |
| VDD | Power | 3.0V | Main power supply |
| GND | Ground | 0V | Common ground |

#### Front-light

The front-light consists of 9 LEDs (2.8V typical) controlled by a MOSFET. The MOSFET blocks current by default (low state = off), allowing PWM control to adjust light intensity.

**Characteristics:**
- 9 integrated LEDs
- Voltage: 2.8V typical
- Control: MOSFET with PWM
- Consumption: ~20-30mA at maximum intensity
- Interface: GPIO with PWM (Hardware or Software PWM)

**PWM Control:**
- Recommended frequency: 1-10 kHz (avoid flicker)
- Resolution: 8-12 bits (256-4096 levels)
- Duty cycle: 0-100% (0% = off, 100% = max)

#### Documentation

- Datasheet: `GDEY037T03-FT21.pdf`
- Website: [www.good-display.com](https://www.good-display.com)

---

### 2. BME280 Sensor

**Manufacturer**: Bosch  
**Model**: BME280  
**Type**: Environmental sensor (Temperature, Humidity, Pressure)

#### Specifications

| Parameter | Value |
|-----------|-------|
| Interface | I2C (or SPI) |
| I2C Addresses | 0x76 or 0x77 (depending on configuration) |
| Temperature | -40°C to +85°C |
| Temperature Accuracy | ±1°C |
| Humidity | 0-100% RH |
| Humidity Accuracy | ±3% RH |
| Pressure | 300-1100 hPa |
| Pressure Accuracy | ±1 hPa |
| Voltage | 1.8V to 3.6V |
| Consumption | ~3.6µA (sleep mode) |

#### Connection

| Pin | Function | Raspberry Pi |
|-----|----------|--------------|
| VCC | 3.3V Power | 3.3V |
| GND | Ground | GND |
| SDA | I2C Data | GPIO 2 (SDA) |
| SCL | I2C Clock | GPIO 3 (SCL) |

#### Documentation

- Datasheet: Available on Bosch website
- Amazon module: [BME280](https://www.amazon.fr/Gy-bme280-num%C3%A9rique-pr%C3%A9cision-barom%C3%A9trique-Temp%C3%A9rature/dp/B077PNKCQ6)

---

### 3. PN532 NFC Module

**Manufacturer**: NXP  
**Model**: PN532  
**Type**: 13.56 MHz NFC controller

#### Specifications

| Parameter | Value |
|-----------|-------|
| Interface | I2C (or SPI/UART depending on configuration) |
| I2C Address | 0x24 (typical, may vary by module) |
| Protocols | MIFARE Classic, NTAG21x, ISO14443 Type A/B |
| Range | ~5cm |
| Voltage | 3.3V or 5V (depending on module) |
| Consumption | ~15mA (active mode) |
| Frequency | 13.56 MHz |

#### Characteristics

- Support for multiple NFC protocols
- Polling mode for automatic tag detection
- NFC tag read/write
- Compatible with most standard NFC tags

#### Connection

| Pin | Function | Raspberry Pi |
|-----|----------|--------------|
| VCC | Power | 3.3V or 5V (depending on module) |
| GND | Ground | GND |
| SDA | I2C Data | GPIO 2 (SDA) |
| SCL | I2C Clock | GPIO 3 (SCL) |

**Note**: Verify that the module is configured in I2C mode (jumpers/selectors on breakout board).

#### Documentation

- Datasheet: Available on NXP website
- Amazon modules:
  - [NFC Module 1](https://www.amazon.fr/dp/B0FB95HMMC/)
  - [NFC Module 2](https://www.amazon.fr/dp/B0DJP3987K/)

---

### 4. WS2812B LED Strip (Optional)

**Type**: Addressable RGB LED  
**Interface**: GPIO (proprietary protocol)

#### Specifications

| Parameter | Value |
|-----------|-------|
| Interface | GPIO (1-wire) |
| Pin | GPIO 21 (proposed) |
| Voltage | 5V |
| Consumption | ~60mA per LED (white max) |
| Protocol | WS2812B (critical timing) |

#### Notes

- Requires dedicated library (rpi_ws281x, neopixel)
- Precise timing required (DMA recommended)
- External power supply recommended for >10 LEDs

---

### 5. PWM Fan (Optional)

**Type**: 5V fan with PWM control

#### Specifications

| Parameter | Value |
|-----------|-------|
| Interface | GPIO PWM |
| Pin | GPIO 18 (Hardware PWM) |
| Voltage | 5V |
| Control | 0-100% speed |

#### Notes

- Uses Raspberry Pi hardware PWM
- Regulation based on temperature (BME280 or CPU)
- Can be manually controlled via HA

---

## Raspberry Pi Configuration

### Required config.txt

```ini
# Enable I2C
dtparam=i2c_arm=on
dtparam=i2c1=on

# Enable SPI
dtparam=spi=on

# PWM for fan (GPIO 18)
dtoverlay=pwm,pin=18,func=2
```

### Linux Devices

| Device | Usage |
|--------|-------|
| `/dev/i2c-1` | Main I2C bus (touch, BME280, NFC) |
| `/dev/spidev0.0` | SPI bus (E-Paper display) |
| `/dev/gpiomem` | GPIO access (LED, front-light, control) |

---

## Connection Diagram (To be completed)

```
Raspberry Pi 4/5
├── SPI0
│   ├── MOSI (GPIO 10) ──> E-Paper Display (Data)
│   ├── SCLK (GPIO 11) ──> E-Paper Display (Clock)
│   └── CE0  (GPIO 8)  ──> E-Paper Display (CS)
│
├── I2C1
│   ├── SDA (GPIO 2) ──> Touch FT6336U, BME280, NFC
│   └── SCL (GPIO 3) ──> Touch FT6336U, BME280, NFC
│
├── GPIO
│   ├── GPIO 18 ──> PWM Fan
│   ├── GPIO 21 ──> WS2812B LED (optional)
│   └── GPIO (TBD) ──> E-Paper Display (DC, Reset, BUSY, Front-light PWM)
│
└── Power
    ├── 3.3V ──> BME280, Display (VDD)
    ├── 5V ──> Fan, LED (optional)
    └── GND ──> All components
```

---

## Estimated Power Consumption

| Component | Consumption | Notes |
|-----------|-------------|-------|
| E-Paper Display (standby) | 34µA | Deep sleep: 1.1µA |
| E-Paper Display (refresh) | ~50mA | During 2-3 seconds |
| Front-light | ~20-30mA | 9 LEDs at 2.8V (controlled by MOSFET PWM) |
| BME280 | ~3.6µA | Sleep mode |
| Touch FT6336U | ~10µA | Sleep mode |
| NFC (PN532) | ~15mA | Active mode |
| WS2812B LED | ~60mA/LED | White max |
| Fan | ~100-200mA | 5V PWM |

**Total estimated (standby)**: ~50µA (excellent for embedded applications)  
**Total estimated (active)**: ~200-300mA (depending on active components)

---

## Important Notes

1. **E-Paper**: Refresh is slow (~2-3s), adapt user interface accordingly
2. **I2C**: All I2C components share the same bus, verify addresses
3. **Power supply**: Raspberry Pi can provide 3.3V and 5V, but check current limits
4. **Front-light**: Controlled via MOSFET with PWM, 2.8V typical voltage (9 integrated LEDs)

---

*Last updated: 2026-01-17*
