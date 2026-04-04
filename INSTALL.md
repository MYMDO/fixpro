# FiXPro Installation Guide

This guide covers all installation methods for FiXPro universal programmer.

## Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Firmware Installation](#firmware-installation)
- [Web Interface Setup](#web-interface-setup)
- [CLI Tool Installation](#cli-tool-installation)
- [Verifying Installation](#verifying-installation)

---

## Hardware Requirements

### Required Components

| Component | Description |
|-----------|-------------|
| Raspberry Pi Pico | or Pico W |
| USB Cable | USB-A to USB-Micro (with data support) |
| Computer | Windows, macOS, or Linux |
| Browser | Chrome 89+, Edge 89+, or Opera 76+ |

### Optional Components

| Component | Purpose |
|-----------|---------|
| SOIC-8 Test Clip | In-circuit programming |
| Breadboard | Prototype connections |
| Jumper Wires | Connections |
| ZIF Socket | Easy chip insertion |

---

## Firmware Installation

### Method 1: Pre-built Firmware (Recommended)

1. **Download the firmware**
   - Download: [`FiXPro_platformio.uf2`](FiXPro_platformio.uf2)

2. **Enter BOOTSEL mode**
   - Hold the BOOTSEL button on Pico
   - Connect USB cable while holding
   - Release BOOTSEL when drive appears

3. **Copy firmware**
   - Drag `FiXPro_platformio.uf2` to RPI-RP2 drive
   - Wait for automatic reboot

4. **Verify**
   - LED should blink rapidly
   - New USB device should appear

### Method 2: PlatformIO Build

1. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

2. **Clone repository**
   ```bash
   git clone https://github.com/MYMDO/fixpro.git
   cd fixpro
   ```

3. **Build firmware**
   ```bash
   cd firmware/platformio
   pio run
   ```

4. **Upload**
   - Option A: `pio run --target upload`
   - Option B: Manual copy via BOOTSEL

### Method 3: Pico SDK Build

1. **Install prerequisites**
   ```bash
   sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi
   ```

2. **Install Pico SDK**
   ```bash
   git clone --depth 1 --branch 2.0.0 https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
   cd ~/pico-sdk && git submodule update --init
   ```

3. **Build**
   ```bash
   cd fixpro/firmware
   mkdir build && cd build
   cmake .. -DPICO_SDK_PATH=~/pico-sdk
   make -j$(nproc)
   ```

4. **Flash**
   - Copy `fixpro.uf2` to Pico via BOOTSEL

---

## Web Interface Setup

### Access

1. **Open in browser**
   ```
   docs/index.html
   ```

2. **Connect device**
   - Click **⚡ Connect** button
   - Select FiXPro device from list
   - Click **PING** to test

### Browser Requirements

| Browser | Version |
|---------|---------|
| Chrome | 89+ |
| Edge | 89+ |
| Opera | 76+ |
| Brave | 1.36+ |
| Vivaldi | 5.0+ |

> **Note**: Firefox does not currently support Web Serial API.

### Linux Permissions

If device is not accessible, add udev rule:

```bash
# Create rule file
sudo nano /etc/udev/rules.d/50-fixpro.rules
```

Add this content:
```
SUBSYSTEM=="tty", ATTRS{idVendor}=="2e8a", MODE="0666", GROUP="dialout"
```

Then reload rules:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Logout and login again for group changes to take effect.

---

## CLI Tool Installation

### Python Installation

```bash
# Install Python 3.8+
python3 --version

# Install pip
sudo apt install python3-pip  # Ubuntu/Debian
brew install python3          # macOS
```

### CLI Installation

```bash
# Clone repository
git clone https://github.com/MYMDO/fixpro.git
cd fixpro

# Install CLI tool
pip install pyserial

# Run CLI
python3 -m host.cli.fixpro --help
```

### Verify CLI

```bash
$ python3 -m host.cli.fixpro --help
usage: fixpro [-h] [--port PORT] [--baud BAUD] ...

FiXPro Universal Programmer CLI

optional arguments:
  --port PORT    Serial port device
  --baud BAUD    Baud rate (default: 115200)
  --help         Show this help message
```

---

## Verifying Installation

### 1. Check USB Connection

```bash
# Linux
ls -la /dev/ttyACM*

# macOS
ls -la /dev/cu.usbmodem*

# Windows
# Check Device Manager for "FiXPro"
```

### 2. Test with Web Interface

1. Open `docs/index.html`
2. Click Connect
3. Select FiXPro device
4. Click PING button
5. Should see "CAFE" response

### 3. Test with CLI

```bash
python3 -m host.cli.fixpro --port /dev/ttyACM0 ping

# Expected output:
# Sending: PING
# Received: CAFE
# Connection OK
```

---

## Troubleshooting

### Device Not Appearing

1. Try different USB cable
2. Try different USB port
3. Press BOOTSEL and reconnect
4. Check USB power settings

### Permission Denied (Linux)

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Logout and login, or:
newgrp dialout
```

### Web Interface Not Connecting

1. Close other serial monitor applications
2. Ensure no other program is using the port
3. Try refreshing the page
4. Check browser console for errors

### CLI Port Busy

```bash
# Kill any process using the port
sudo lsof /dev/ttyACM0
sudo kill <PID>
```

---

## Next Steps

- Read the [README.md](README.md) for usage instructions
- Browse supported chips in the web interface
- Try reading your first flash chip!

---

## Support

- [GitHub Issues](https://github.com/MYMDO/fixpro/issues)
- [GitHub Discussions](https://github.com/MYMDO/fixpro/discussions)
