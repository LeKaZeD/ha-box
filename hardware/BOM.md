# Bill of Materials – HA Box V0.1

This document lists all components required to build the HA Box V0.1.

Prices are indicative and based on Amazon France at time of writing. Links may
change; always verify the reference/model before ordering.

---

## 1. Raspberry Pi 5 stack

| Component | Model / Reference | Qty | Unit price | Link |
|-----------|-------------------|-----|-----------|------|
| Raspberry Pi 5 | Pi 5 – 8 GB *(recommended)* | 1 | €108 | [Amazon](https://amazon.fr/dp/B0CK2FCG1K/) |
| *(alternative)* | Pi 5 – 16 GB | 1 | €170 | [Amazon](https://www.amazon.fr/dp/B0DSPYPKRG/) |
| Active cooler | Raspberry Pi Active Cooler (official) | 1 | €11 | [Amazon](https://www.amazon.fr/dp/B0CLXZBR5P/) |
| Power supply | Official Raspberry Pi 5 USB-C PSU – 27 W | 1 | €17 | [Amazon](https://www.amazon.fr/dp/B0CN3MRV16/) |
| M.2 + port extension hat | Waveshare PCIe to M.2 Multifunctional Adapter | 1 | €17 | [Amazon](https://www.amazon.fr/dp/B0G2SFFDTR/) |
| NVMe SSD | M.2 NVMe 128 GB (2280) | 1 | €32 | [Amazon](https://www.amazon.fr/dp/B0DPPRV85Q/) |
| Internal USB hub | Waveshare PCIe to USB 3.2 / 2.5G ETH / M.2 NVMe Adapter | 1 | €58 | [Amazon](https://www.amazon.fr/dp/B0FD6QHTQV/) |

**Notes:**
- The **Waveshare Multifunctional Adapter** routes the Pi's HDMI and USB-C ports to the rear of the enclosure and provides the M.2 NVMe slot.
- The **Waveshare USB / ETH / NVMe Adapter** adds an internal USB port used to host the Sonoff dongle inside the enclosure.
- The official active cooler replaces the standard heatsink; it mounts on the Pi 5 GPIO header side.

**Subtotal (8 GB config): ~€243**

---

## 2. haboxesp PCB + interface

The haboxesp is the custom PCB for this project. It hosts an ESP32 module and
connects the display, sensors, fan, and button. The PCB is available for
purchase (link coming soon).

| Component | Model / Reference | Qty | Unit price | Link |
|-----------|-------------------|-----|-----------|------|
| haboxesp PCB (assembled) | haboxesp V0.1 | 1 | TBD | Coming soon |
| ESP32 module | ESP32-WROOM-32 (38-pin) | 1 | ~€10 | — |
| E-Paper display | GDEY037T03-FT21 (3.7", 240×416, integrated FT6336U touch) | 1 | €19 | [buyepaper.com](https://buyepaper.com/products/gdey037t03-ft21) |
| Environmental sensor | BME280 (temperature, humidity, pressure) | 1 | ~€3 | [Amazon](https://www.amazon.fr/dp/B0BJF3L9CS/) |
| Fan | Noctua NF-A6x15 5V PWM | 1 | €17 | [Amazon](https://www.amazon.fr/dp/B0DJP3987K/) |
| Push button | 4-pin tact switch (momentary) | 1 | ~€2 | [Amazon](https://www.amazon.fr/dp/B09WYRHPDL/) |

**Notes:**
- The **haboxesp PCB** includes the buzzer (HYT-1205), TMP102 case temperature sensor, FPC connectors for the display and front-light, I2C connectors for BME280, and all passive components. No additional PCB components need to be sourced separately.
- The **BME280** module must include humidity support. Do not substitute with BMP280 (no humidity).
- The **LED strip** (WS2812B) was evaluated but is **not used in V0.1**. The haboxesp PCB retains the connector for future use.

**Subtotal: ~€51 + PCB price TBD**

---

## 3. Radio modules

| Component | Model / Reference | Qty | Unit price | Link |
|-----------|-------------------|-----|-----------|------|
| Zigbee + Thread coordinator | Sonoff ZigBee 3.0 USB Dongle Plus (EFR32MG21) | 1 | €23 | [Amazon](https://www.amazon.fr/dp/B09KXTCMSC/) |
| 433 MHz transceiver | CC1101 module (SPI) | 1 | ~€5 | TBD – driver experimental (SPI + RFLink TCP server implemented) |

**Notes:**
- The **Sonoff dongle** is plugged into the internal USB port provided by the Waveshare USB adapter, keeping it inside the enclosure.
- The **CC1101** module connects to the Pi 5 via SPI. The SPI driver and RFLink-compatible TCP server are implemented (experimental); HA entity/event publishing is not yet done.

**Subtotal: ~€28**

---

## 4. Enclosure

The enclosure combines 3D-printed structural parts with three finishing materials:
a wood side panel, a plexiglass front window (display area), and a steel front panel.

| | |
|---|---|
| ![Front](../3D%20plan/images/Home_assistant_box_Face.png) | ![Inside](../3D%20plan/images/Home_assistant_box_inside.png) |

*Left: assembled front view (wood side panel, plexiglass window, steel front panel). Right: inside view (3D-printed frame, haboxesp PCB, Raspberry Pi).*

### 4a. 3D-printed parts

All printed parts use the STL files in `hardware/3D plan/Box parts/`.

| Component | Reference | Qty | Unit price | Link |
|-----------|-----------|-----|-----------|------|
| Threaded heat inserts (M2.5, OD 3.5 mm) | Assorted kit – 300 pcs | 1 | €12 | [Amazon](https://www.amazon.fr/dp/B0CVVFRMRH/) |
| M2.5 standoffs and screws | Hex standoff assortment – 486 pcs | 1 | €17 | [Amazon](https://www.amazon.fr/dp/B0C1FY87PY/) |
| 3D print filament | PLA or PETG – 1 kg | 1 | ~€20 | — |

**STL parts to print:**

| File | Role | Qty | Notes |
|------|------|-----|-------|
| `Box.stl` | Main structural frame | 1 | Largest part; holds all components |
| `Top.stl` | Top cover | 1 | Fan exhaust slot |
| `Backplate.stl` | Rear panel | 1 | HDMI / USB-C cutouts |
| `Support.stl` | Internal mounting bracket | 1 | PCB and Pi mounting |
| `Pied.stl` | Stand / base | 1 | Single base piece |
| `Button.stl` | Power button cap | 1 | |

**Notes:**
- Recommended material: **PETG** (better heat resistance near fan). PLA is acceptable for prototyping.
- Layer height: 0.2 mm. Infill: 20–30% (see `TODO_GETTING_STARTED.md` for per-part settings).
- Threaded inserts used are **M2.5, OD 3.5 mm**. The entire model is designed around this dimension. Do not substitute with a different OD.

### 4b. Finishing materials

| Material | Role | Dimensions | Notes |
|----------|------|-----------|-------|
| Wood panel | Left side panel – structural and decorative | TBD (from Fusion 360 model) | Thin veneer or solid wood sheet; has a cutout for the plexiglass window |
| Plexiglass / acrylic sheet | Window inset in the wood panel | TBD (from Fusion 360 model) | Transparent or frosted; sits inside the wood cutout on the left side panel |
| Steel sheet | Front face panel | TBD (from Fusion 360 model) | Matte finish; laser-cut or cut-to-size |

The wood panel and the plexiglass window are on the **same side** (left panel): the wood forms the surround and the acrylic fills the cutout as a window. The steel sheet covers the **front face** separately.

**Note:** Exact dimensions for all three materials are derived from the Fusion 360 source file (`hardware/3D plan/Fusion/Home assistant box.f3z`). Refer to the model or the assembly video for cutting dimensions.

**Subtotal: ~€49 + finishing materials (TBD)**

---

## Total (V0.1 – Pi 5 8 GB)

| Section | Subtotal |
|---------|---------|
| Raspberry Pi 5 stack | ~€243 |
| haboxesp PCB + interface | ~€51 + PCB TBD |
| Radio modules | ~€28 |
| 3D enclosure hardware | ~€49 |
| **Total (excl. PCB)** | **~€371** |

---

## Not included

- **WS2812B LED strip** – evaluated, not used in V0.1. Supported by haboxesp PCB connector for future use. [Amazon](https://www.amazon.fr/dp/B0D5CJK1YS/)
- **CC1101 module** – SPI driver + RFLink TCP server experimental; HA entity publishing not yet done.
- Tools (soldering iron, screwdrivers, calipers, 3D printer).
