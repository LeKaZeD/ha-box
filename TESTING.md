# Testing HA Box App on Home Assistant OS

This guide explains how to test the HA Box App on Home Assistant OS.

## Prerequisites

1. **Home Assistant OS** installed on Raspberry Pi 4 or 5
2. **SSH access** to your Home Assistant instance
3. **Samba** or **SSH & Web Terminal** App enabled (for file transfer)
4. **Git** installed on your development machine

## Method 1: Local App (Recommended for Development)

### Step 1: Prepare Your App

1. **Create icon and logo** (optional but recommended):
   - Create `ha-box/icon.png` (256x256 pixels)
   - Create `ha-box/logo.png` (256x256 pixels)
   - Or use placeholder images for now

### Step 2: Transfer Files to Home Assistant

#### Option A: Using Samba Share

1. Enable the **Samba share** App in Home Assistant
2. Access the share from your computer (usually `\\homeassistant\config` or `smb://homeassistant.local`)
3. Navigate to `addons/` (or `addons/local/`, create if it does not exist)
4. Copy the entire `ha-box` folder to `addons/ha-box/` (or `addons/local/ha-box/`)

#### Option B: Using SSH

1. Enable the **SSH & Web Terminal** App in Home Assistant
2. SSH into your Home Assistant instance:
   ```bash
   ssh root@homeassistant.local
   # or
   ssh root@<your-ha-ip>
   ```
3. Create the local Apps directory:
   ```bash
   mkdir -p /config/addons
   ```
4. Transfer files using `scp` from your development machine:
   ```bash
   scp -r ha-box root@homeassistant.local:/config/addons/
   ```

### Step 3: Refresh App Store

1. In Home Assistant, go to **Settings** → **Apps** → **App Store**
2. Click the **⋮** (three dots) menu in the top right
3. Click **Check for updates**
4. Wait a few seconds and refresh the page (F5)

### Step 4: Install the App

1. You should now see "HA Box" in the App store
2. Click on **HA Box**
3. Click **Install**
4. Wait for installation to complete
5. Go to **Configuration** tab and configure options
6. Click **Start**

### Optional: Local Development on VM (amd64, No GPIO)

When running on a Home Assistant OS VM without real hardware (no UART, no GPIO, no SPI):

1. Edit `ha-box/config.yaml`:
   - Ensure architectures include both:
     ```yaml
     arch:
       - aarch64
       - amd64
     ```
   - Disable GPIO and kernel modules to avoid VM crashes:
     ```yaml
     gpio: false
     kernel_modules: false
     ```
   - For local builds, comment out the `image` line so the Supervisor builds from the local `Dockerfile`:
     ```yaml
     #image: "ghcr.io/YOUR_USERNAME/{arch}-addon-ha-box"
     ```
2. Copy the updated `ha-box` folder to the HAOS VM as described above.
3. Use **Check for updates** in the App Store, then install and start **HA Box**.

## Method 2: Git Repository (For Production)

### Step 1: Push to GitHub

1. Create a GitHub repository
2. Push your code:
   ```bash
   git remote add origin https://github.com/LeKaZeD/ha-box.git
   git push -u origin main
   ```

### Step 2: Add Repository to Home Assistant

1. In Home Assistant, go to **Settings** → **Apps** → **App Store**
2. Click the **⋮** menu → **Repositories**
3. Add repository URL: `https://github.com/LeKaZeD/ha-box`
4. Click **Add**
5. Refresh the page

### Step 3: Install the App

1. Find **HA Box** in the App store
2. Click **Install**
3. Configure and start

## Method 3: Direct Build and Test (Advanced)

### Step 1: Build the Docker Image Locally

```bash
cd ha-box
docker build -t ha-box:local --build-arg BUILD_ARCH=aarch64 .
```

### Step 2: Test the Container

```bash
docker run --rm -it \
  --device=/dev/ttyAMA0 \
  --device=/dev/serial0 \
  --device=/dev/gpiochip0 \
  ha-box:local \
  python3 /usr/bin/ha-box/main.py
```

## Troubleshooting

### App Not Appearing

- Check that `repository.yaml` is in the correct location
- Verify the `ha-box` folder structure matches the expected format
- Check Home Assistant logs: **Settings** → **System** → **Logs**
- Restart Home Assistant Supervisor

### Installation Fails

- Check Docker logs: `docker logs <container-id>`
- Verify all required files are present
- Check `config.yaml` syntax is valid YAML
- Ensure `Dockerfile` is correct

### App Crashes on Start

- Check App logs: **HA Box** → **Logs** tab
- Verify hardware permissions in `config.yaml`
- If RF433 is enabled: verify SPI is enabled in `/mnt/boot/config.txt` (`dtparam=spi=on`)
- Verify Python dependencies are installed correctly

### ESP32 Not Detected / UART Not Working

1. **Enable UART** on Raspberry Pi (HAOS):

   `/mnt/boot/config.txt` cannot be edited via SSH or the web terminal. Edit it physically — connect a keyboard and screen to the Pi, or mount the SD card / NVMe on a computer. See [docs/HARDWARE.md](docs/HARDWARE.md#pi-configtxt-haos) for the exact lines to add (differs between Pi 4 and Pi 5).

2. **Verify UART device exists**:
   ```bash
   ls -l /dev/serial0 /dev/ttyAMA0
   ```

3. **Verify GPIO chardev**:
   ```bash
   ls -l /dev/gpiochip0
   ```

## Development Workflow

1. **Make changes** to your code
2. **Transfer files** to Home Assistant (Method 1, Step 2)
3. **Restart the App** in Home Assistant
4. **Check logs** for errors
5. **Repeat** as needed

## Initial ESP32 Firmware Flash

The App bundles an ESP32 firmware binary and flashes it automatically via OTA when the version differs. However, for a **brand-new ESP32** (no firmware yet), you need to flash it manually once via USB.

### Requirements

- Arduino IDE 2.x with ESP32 board package installed
- USB-to-Serial adapter (or direct USB connection if supported by your board)
- GxEPD2 library installed via Library Manager
- `gdey/` subfolder copied from the GxEPD2 library into `ESPHOMEASSISTANT/` (see `ha-box-esp-fw/README.md`)

### Steps

1. Open `ha-box-esp-fw/ESPHOMEASSISTANT/ESPHOMEASSISTANT.ino` in Arduino IDE
2. Set board to `ESP32 Dev Module`, upload speed `115200`, partition scheme `Default 4MB with spiffs`
3. Connect the ESP32 via USB and select the correct port
4. Click **Upload**
5. Once flashed, disconnect USB and reconnect the ESP32 to the Pi via UART

From this point on, the App will handle firmware updates automatically (OTA via UART0 when the bundled version differs from what is running).

## Next Steps

Once the App is running:

1. Check logs to verify startup and ESP32 connection (`READY` handshake)
2. Verify sensor entities appear in Home Assistant (BME280 × 3, TMP102)
3. Check OTA: if ESP32 firmware version differs from bundled, flash should trigger automatically
4. Test configuration changes (fan curve, OTA GPIO pins)

---

*For more information, see the [Home Assistant Apps Documentation](https://developers.home-assistant.io/docs/apps/)*
