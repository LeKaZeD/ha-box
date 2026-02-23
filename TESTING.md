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

When running on a Home Assistant OS VM without real GPIO/I2C/SPI hardware:

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
   git remote add origin https://github.com/YOUR_USERNAME/ha-box.git
   git push -u origin main
   ```

### Step 2: Add Repository to Home Assistant

1. In Home Assistant, go to **Settings** → **Apps** → **App Store**
2. Click the **⋮** menu → **Repositories**
3. Add repository URL: `https://github.com/YOUR_USERNAME/ha-box`
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
  --device=/dev/i2c-1 \
  --device=/dev/spidev0.0 \
  --device=/dev/gpiomem \
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
- Check that I2C/SPI are enabled in `/mnt/boot/config.txt`
- Verify Python dependencies are installed correctly

### Hardware Not Detected

1. **Enable I2C and SPI** on Raspberry Pi:
   ```bash
   # SSH into Home Assistant
   ssh root@homeassistant.local
   
   # Edit config.txt
   nano /mnt/boot/config.txt
   
   # Add these lines:
   dtparam=i2c_arm=on
   dtparam=i2c1=on
   dtparam=spi=on
   
   # Save and reboot
   reboot
   ```

2. **Verify devices exist**:
   ```bash
   ls -l /dev/i2c-1
   ls -l /dev/spidev0.0
   ls -l /dev/gpiomem
   ```

3. **Check I2C devices**:
   ```bash
   i2cdetect -y 1
   ```

## Development Workflow

1. **Make changes** to your code
2. **Transfer files** to Home Assistant (Method 1, Step 2)
3. **Restart the App** in Home Assistant
4. **Check logs** for errors
5. **Repeat** as needed

## Next Steps

Once the App is running:

1. Check logs to verify startup
2. Test hardware detection (if hardware is connected)
3. Verify Home Assistant API connection
4. Test configuration changes
5. Continue development on Phase 3 (hardware drivers)

---

*For more information, see the [Home Assistant Apps Documentation](https://developers.home-assistant.io/docs/apps/)*
