# Security Policy - HA Box App

## Security Rating

This App is designed to achieve a **security rating of 6/6** (maximum security) according to Home Assistant App security standards.

**Note**: Home Assistant's official security rating scale is **1-6**, where 6 is the maximum. All Apps start at 5/6, then points are added/subtracted based on configuration choices.

## Security Features

### Protection Mode

- **Status**: Enabled (default)
- **Rationale**: Protection mode provides isolation and security restrictions. This App does not require disabling protection mode.

### Permissions

The App requires the following permissions for hardware access:

#### Hardware Access

The Pi App communicates with the ESP32 over **UART** and uses **GPIO** for OTA flashing only. All sensors (BME280, TMP102, touch) are connected directly to the ESP32 — the Pi does not access I2C or any display hardware.

- **GPIO**: Required for OTA flashing (IO0 and EN pins on the ESP32, via `gpiod` chardev — not legacy `/dev/gpiomem`)
- **Devices**:
  - `/dev/ttyAMA0` / `/dev/serial0`: UART for ESP32 communication and OTA flashing
  - `/dev/gpiochip0`: GPIO chardev for OTA pin control (IO0, EN)
  - `/dev/spidev0.0`: SPI for CC1101 433 MHz module (only active when RF433 is enabled in config)

#### File System Access
- **Read-only**: Configuration files, Python code
- **Read-write**: Logs only (`/var/log/**`)
- **No host filesystem access**: App is fully containerized

### AppArmor Profile

A custom AppArmor profile (`apparmor.txt`) restricts the App to:
- Only necessary hardware devices
- Network access limited to Supervisor API
- No access to host filesystem
- Minimal file system permissions

**Profile Details**:
- Allows execution of s6-overlay init system (`/init`, `/run/s6/**`, `/usr/bin/s6-*`) - required for App startup
- Allows execution of basic system binaries (`/bin/sh`, `/bin/bash`, `/usr/bin/with-contenv`) - required by s6-overlay
- Allows UART, GPIO chardev, and SPI device access (hardware requirement)
- Allows network access for Supervisor API communication
- Allows reading and executing Python code from `/usr/bin/ha-box/**`
- Allows reading and executing service scripts from `/etc/services.d/**`
- Allows reading configuration from `/data/options.json`
- Allows writing logs to `/var/log/**`
- Blocks all other system access

### Network Security

- **No host network**: App uses container networking
- **No exposed ports**: Communication via Supervisor API only (except RF433 TCP server on port 5557 when enabled)
- **No ingress**: No web interface
- **API authentication**: Uses Supervisor token (automatic, secure)

### Container Security

- **No privileged mode**: App runs with standard user permissions
- **No SYS_ADMIN**: No system administration capabilities
- **Isolated filesystem**: Only mapped directories accessible
- **Minimal base image**: Uses official Home Assistant base images

## Threat Model

### Considered Threats

1. **Hardware Access Abuse**
   - **Mitigation**: AppArmor restricts to specific devices only
   - **Risk**: Low - only hardware-specific devices accessible

2. **Network Exposure**
   - **Mitigation**: No exposed ports by default; RF433 TCP server (port 5557) only active when explicitly enabled
   - **Risk**: Low - RF433 server is LAN-local only

3. **File System Access**
   - **Mitigation**: Read-only access to code, write only to logs
   - **Risk**: Low - no host filesystem access

4. **Privilege Escalation**
   - **Mitigation**: No privileged mode, no SYS_ADMIN
   - **Risk**: Low - standard container permissions

### Known Limitations

- **Hardware dependencies**: Requires UART device and GPIO chardev
- **RF433 TCP server**: Port 5557 is exposed on the LAN when RF433 is enabled; no authentication (by design — matches RFLink protocol)
- **Image signing**: Not yet implemented (planned for production releases)

## Security Best Practices for Users

1. **Keep App updated**: Regular updates include security patches
2. **Review configuration**: Only enable RF433 if you have the CC1101 hardware
3. **Monitor logs**: Check logs for unusual activity
4. **Hardware security**: Ensure physical access to Raspberry Pi is secured

## Reporting Security Issues

If you discover a security vulnerability, please:
1. **Do NOT** create a public issue
2. Open a private security advisory on the GitHub repository
3. Include details of the vulnerability
4. Allow time for fix before public disclosure

## Security Updates

- **Dependencies**: Updated regularly via requirements.txt
- **Base image**: Uses Home Assistant base images (automatically updated)
- **Python packages**: Pinned versions for stability and security

## Compliance

This App follows Home Assistant App security guidelines:
- ✅ Protection mode enabled
- ✅ No privileged mode
- ✅ No host network
- ✅ Custom AppArmor profile
- ✅ Minimal permissions
- ✅ Container isolation
- ✅ Presentation assets (icon, logo)
- ⚠️ Image signing (planned for production)

## Security Rating Breakdown

| Security Feature | Status | Notes |
|------------------|--------|-------|
| Protection Mode | ✅ Enabled | Default, not disabled |
| Privileged Mode | ✅ Not used | Standard permissions |
| Host Network | ✅ Not used | Container networking |
| AppArmor | ✅ Custom profile | Hardware-specific permissions |
| Minimal Permissions | ✅ Yes | Only hardware devices |
| Image Signing | ⚠️ Planned | For production releases |
| Documentation | ✅ Complete | This file |
| Presentation | ✅ Complete | Icons and logo included |

**Current Rating**: 6/6 (Maximum)  
**Breakdown**: Base 5/6 + AppArmor profile (+1) = 6/6

---

*Last updated: 2026-04-17*
