# Security Policy - HA Box Add-on

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
- **GPIO**: Required for controlling LEDs, fan PWM, and display control pins
- **Kernel Modules**: Required for I2C and SPI bus access
- **Devices**: 
  - `/dev/i2c-1`: I2C bus for sensors (BME280, NFC, touch)
  - `/dev/spidev0.0`: SPI bus for E-Paper display
  - `/dev/gpiomem`: GPIO memory access for pin control

#### File System Access
- **Read-only**: Configuration files, Python code
- **Read-write**: Logs only (`/var/log/**`)
- **No host filesystem access**: Add-on is fully containerized

### AppArmor Profile

A custom AppArmor profile (`apparmor.txt`) restricts the App to:
- Only necessary hardware devices
- Network access limited to Supervisor API
- No access to host filesystem
- Minimal file system permissions

**Profile Details**:
- Allows execution of s6-overlay init system (`/init`, `/run/s6/**`, `/usr/bin/s6-*`) - required for App startup
- Allows execution of basic system binaries (`/bin/sh`, `/bin/bash`, `/usr/bin/with-contenv`) - required by s6-overlay
- Allows I2C/SPI/GPIO device access (hardware requirement)
- Allows network access for Supervisor API communication
- Allows reading and executing Python code from `/usr/bin/ha-box/**`
- Allows reading and executing service scripts from `/etc/services.d/**`
- Allows reading configuration from `/data/options.json`
- Allows writing logs to `/var/log/**`
- Blocks all other system access

### Network Security

- **No host network**: Add-on uses container networking
- **No exposed ports**: Communication via Supervisor API only
- **No ingress**: Currently no web interface (may be added in future)
- **API authentication**: Uses Supervisor token (automatic, secure)

### Container Security

- **No privileged mode**: Add-on runs with standard user permissions
- **No SYS_ADMIN**: No system administration capabilities
- **Isolated filesystem**: Only mapped directories accessible
- **Minimal base image**: Uses official Home Assistant base images

## Threat Model

### Considered Threats

1. **Hardware Access Abuse**
   - **Mitigation**: AppArmor restricts to specific devices only
   - **Risk**: Low - only hardware-specific devices accessible

2. **Network Exposure**
   - **Mitigation**: No exposed ports, no host network
   - **Risk**: Low - communication via Supervisor API only

3. **File System Access**
   - **Mitigation**: Read-only access to code, write only to logs
   - **Risk**: Low - no host filesystem access

4. **Privilege Escalation**
   - **Mitigation**: No privileged mode, no SYS_ADMIN
   - **Risk**: Low - standard container permissions

### Known Limitations

- **Hardware dependencies**: Requires GPIO/kernel modules (hardware App limitation)
- **No ingress**: Currently no web interface (future enhancement)
- **Image signing**: Not yet implemented (planned for production releases)

## Security Best Practices for Users

1. **Keep App updated**: Regular updates include security patches
2. **Review configuration**: Only enable features you need
3. **Monitor logs**: Check logs for unusual activity
4. **Hardware security**: Ensure physical access to Raspberry Pi is secured

## Reporting Security Issues

If you discover a security vulnerability, please:
1. **Do NOT** create a public issue
2. Email security concerns to: [Your security email]
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
- ⚠️ Ingress (not applicable currently - no web UI)

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

**Additional Security Features Available** (all result in 6/6):
- Ingress: +2 points (if web UI added)
- CodeNotary signing: +1 point (for production)

---

*Last updated: 2026-01-20*
