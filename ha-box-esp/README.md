# ha-box-esp — ESP32 Firmware

C++ firmware for the haboxesp board. Manages the E-Paper display, touch, sensors (BME280, TMP102), fan, buzzer, and power button. Communicates with the Pi add-on over UART0 using the ASCII protocol defined in [docs/ESP32_RASP_COM.md](../docs/ESP32_RASP_COM.md).

## Build & Flash

Uses [PlatformIO](https://platformio.org). Dependencies (GxEPD2 display driver, etc.) are fetched automatically from `platformio.ini`.

```bash
# Install PlatformIO CLI or open the project in VS Code with the PlatformIO extension
pio run --target upload
```

The OTA update is handled automatically by the add-on on every startup if the bundled firmware version differs from the ESP32 reported version.

## Releasing a new firmware version

1. Increment `FIRMWARE_VERSION` in [`src/config.h`](src/config.h)
2. Build: `pio run` → binary at `.pio/build/upesy_wroom/firmware.bin`
3. Copy to the add-on firmware folder with the version in the filename:
   ```
   cp .pio/build/upesy_wroom/firmware.bin ../ha-box/firmware/firmware-{VERSION}.bin
   ```
   The add-on reads the version from the filename (pattern `firmware-*.bin`) and flashes automatically on startup if it differs from the ESP32-reported version.

## Pin configuration

See [`src/config.h`](src/config.h) for all GPIO assignments and [`docs/HARDWARE.md`](../docs/HARDWARE.md) for the full pinout.
