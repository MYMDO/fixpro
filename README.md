# FiXPro — Flash iX Pro

<div align="center">

**Universal Hardware Programmer for SPI Flash, I2C EEPROM, ARM MCU & AVR**

[![License: GPL v3](https://img.shields.io/github/license/MYMDO/fixpro?style=flat-square)](LICENSE)
[![Build](https://img.shields.io/github/actions/workflow/status/MYMDO/fixpro/build.yml?style=flat-square)](https://github.com/MYMDO/fixpro/actions)
[![Release](https://img.shields.io/github/v/release/MYMDO/fixpro?style=flat-square)](https://github.com/MYMDO/fixpro/releases)
[![Web](https://img.shields.io/badge/Web-ready-brightgreen?style=flat-square)](docs/index.html)

*Raspberry Pi RP2040 • Web Serial API • No Drivers Required*

</div>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Hardware Requirements](#hardware-requirements)
- [Firmware Installation](#firmware-installation)
- [Web Interface](#web-interface)
- [CLI Tool](#cli-tool)
- [Protocol Reference](#protocol-reference)
- [Supported Chips](#supported-chips)
- [Building from Source](#building-from-source)
- [Project Structure](#project-structure)
- [Documentation](#documentation)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Overview

FiXPro is an open-source universal hardware programmer built on the **Raspberry Pi RP2040** microcontroller. It provides professional-grade flash programming capabilities through a modern **Web Serial API** interface — no drivers or software installation required.

**FiXPro** stands for "Flash iX Pro" — universal flash programming with professional performance.

### Key Benefits

- **Cross-Platform**: Works on Windows, macOS, Linux via any modern browser
- **No Installation**: Web-based interface requires only Chrome/Edge/Opera
- **Open Source**: Full firmware and web interface source code available
- **High Performance**: Up to 80 MHz SPI, 1 MHz I2C, 25 MHz JTAG/SWD
- **Rich Protocol Support**: SPI, I2C, JTAG, SWD, UPDI, 1-Wire

---

## Features

### Protocol Support

| Protocol | Max Speed | Description | Status |
|----------|-----------|-------------|--------|
| **SPI Flash** | 80 MHz | Winbond, Macronix, GigaDevice, Micron | ✅ Ready |
| **I2C EEPROM** | 1 MHz | 24Cxx, AT24Cxx, M24Cxx series | ✅ Ready |
| **JTAG** | 25 MHz | ARM MCUs, FPGA, RISC-V | ✅ Ready |
| **SWD** | 25 MHz | ARM Cortex debug interface | ✅ Ready |
| **UPDI** | 460.8 kHz | AVR microcontrollers | ✅ Ready |
| **1-Wire** | - | DS18B20, iButton, EEPROM | ✅ Ready |

### Hardware Platform

| Specification | Value |
|---------------|-------|
| Processor | Raspberry Pi RP2040 |
| Architecture | Dual ARM Cortex-M0+ @ 133 MHz |
| RAM | 264 KB |
| Flash | 2 MB (onboard) |
| USB | USB 2.0 Full Speed via CDC |
| GPIO | 26 pins available for programming |
| Voltage | 3.3V logic (5V tolerant inputs) |

### Software Features

- Real-time operation progress with ETA
- Chip auto-detection via JEDEC ID
- 200+ chip database
- Hex editor for buffer viewing
- Hardware control panel
- Batch operations support

---

## Quick Start

### 1. Flash the Firmware

1. Download the latest firmware: [`FiXPro_platformio.uf2`](docs/FiXPro_platformio.uf2)
2. Hold the **BOOTSEL** button on your Raspberry Pi Pico
3. Connect USB cable while holding BOOTSEL
4. Release BOOTSEL — Pico will mount as `RPI-RP2`
5. Copy `FiXPro_platformio.uf2` to the drive

### 2. Connect via Web Interface

1. Open [`docs/index.html`](docs/index.html) in **Chrome**, **Edge**, or **Opera**
2. Click the **⚡ Connect** button
3. Select FiXPro device from the port list
4. Click **PING** to verify connection

### 3. Start Programming

- Select your chip protocol (SPI, I2C, JTAG, etc.)
- Browse the chip library or use auto-detect
- Use Read/Write/Erase operations

---

## Hardware Requirements

### Minimum Requirements

| Component | Specification |
|-----------|---------------|
| Microcontroller | Raspberry Pi Pico / Pico W |
| USB Cable | USB-A to USB-Micro (data capable) |
| Browser | Chrome 89+, Edge 89+, Opera 76+ |

### Optional Hardware

For full functionality, you may need:

- **SOIC-8 Test Clip** — For programming chips in-circuit
- **Breadboard** — For prototype connections
- **Level Shifters** — For 5V target devices
- **ZIF Socket Adapter** — For easier chip insertion

### Pin Connections

| Signal | Pico Pin | Description |
|--------|----------|-------------|
| SPI_MISO | GP16 | SPI data input |
| SPI_MOSI | GP19 | SPI data output |
| SPI_SCK | GP18 | SPI clock |
| FLASH_CS | GP17 | SPI chip select |
| I2C_SDA | GP20 | I2C data |
| I2C_SCL | GP21 | I2C clock |
| GPIO_10-15 | GP10-15 | General purpose I/O |
| LED | GP25 | Status LED (built-in) |

---

## Firmware Installation

### Method 1: Pre-built Firmware (Recommended)

1. Download [`FiXPro_platformio.uf2`](docs/FiXPro_platformio.uf2)
2. Put Pico in BOOTSEL mode
3. Copy UF2 file to Pico

### Method 2: Build from Source

```bash
# Clone repository
git clone https://github.com/MYMDO/fixpro.git
cd fixpro

# Build with PlatformIO
cd firmware/platformio
pio run
```

### Method 3: Pico SDK (Advanced)

```bash
# Install Pico SDK
git clone --depth 1 --branch 2.0.0 https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init

# Build firmware
cd FiXPro/firmware
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=~/pico-sdk
make -j$(nproc)
```

---

## Web Interface

FiXPro includes a professional web interface accessible via **Web Serial API**.

### Features

- **Dashboard** — Real-time system status and chip info
- **Chip Library** — 200+ supported devices with search
- **Operations** — Read, Write, Verify, Erase with progress
- **Hex Editor** — View and edit buffer contents
- **Hardware Control** — Direct GPIO and protocol testing
- **Script Panel** — Batch operations support

### Accessing the Interface

1. Open `docs/index.html` in your browser
2. Click **⚡ Connect**
3. Grant permission to access serial ports
4. Select the FiXPro device

### Hardware Control Panel

When connected, use the control panel for:

| Button | Command | Description |
|--------|---------|-------------|
| PING | `PING` | Connection test → `CAFE` |
| CAPS | `CAPS` | Get capabilities |
| GPIO | `GPIO` | Read GPIO states |
| SPI ID | `SPI_ID` | Read JEDEC ID |
| I2C Scan | `I2C_SCAN` | Scan I2C bus |

---

## CLI Tool

For advanced users, FiXPro includes a Python CLI tool.

### Installation

```bash
pip install pyserial
python -m fixpro.cli --help
```

### Basic Commands

```bash
# Device info
fixpro info

# Detect chip
fixpro detect

# Read flash
fixpro read -o firmware.bin

# Write flash
fixpro write -i firmware.bin --verify

# Erase chip
fixpro erase --chip

# I2C scan
fixpro i2c-scan
```

---

## Protocol Reference

### Text OPUP Protocol

FiXPro uses a simple text-based protocol over USB CDC.

#### Commands

| Command | Response | Description |
|---------|----------|-------------|
| `PING` | `CAFE` | Connection test |
| `CAPS` | `CAPS:i2c,spi,gpio,isp,swd` | Supported protocols |
| `VERSION` | `FiXPro v2.0.0` | Firmware version |
| `GPIO` | `GPIO:000000` | GPIO states (6 bits) |
| `SPI_ID` | `SPI:EF4017` | SPI flash JEDEC ID |
| `I2C_SCAN` | `I2C:50 68` | Found I2C addresses |
| `STATUS` | `STATUS:...` | System status |
| `HELP` | Command list | Show help |

### Response Format

All responses are line-terminated with `\n` (LF).

```
OK                          # Success
ERR:error_message           # Error
DATA:hex_data               # Data response
```

---

## Supported Chips

### SPI Flash (100+ chips)

| Manufacturer | Models |
|--------------|--------|
| Winbond | W25Q80–W25Q512 |
| Macronix | MX25L128–MX25L256 |
| GigaDevice | GD25Q16–GD25Q256 |
| Micron | N25Q128–N25QL256 |
| ISSI | IS25LP128–IS25WP512 |
| Cypress | S25FL128–S25FL512 |

### I2C EEPROM (60+ chips)

| Series | Models |
|--------|--------|
| AT24C | 01–1024 (128b–128KB) |
| M24C | 02–512 |
| FM24C | 04–256 |

### ARM MCUs (50+ targets)

| Family | Examples |
|--------|----------|
| STM32 | F103, F401, F411, G0, L4 |
| Nordic | nRF52, nRF53 |
| NXP | LPC, Kinetis |
| Espressif | ESP32 |

### AVR MCUs

| Series | Models |
|--------|--------|
| ATmega | 8, 168, 328, 2560 |
| ATtiny | 13, 45, 85, 2313, 416 |
| ATxmega | Full series via UPDI |

---

## Building from Source

### Prerequisites

- **PlatformIO** (recommended) or
- **Pico SDK 2.0+** with CMake
- **ARM GCC Toolchain**

### PlatformIO Build

```bash
# Install PlatformIO
pip install platformio

# Build
cd firmware/platformio
pio run

# Upload
pio run --target upload
```

### Pico SDK Build

```bash
export PICO_SDK_PATH=~/pico-sdk
cd firmware
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=$PICO_SDK_PATH
make -j$(nproc)
```

---

## Project Structure

```
FiXPro/
├── firmware/               # RP2040 Firmware
│   ├── platformio/        # PlatformIO project
│   │   ├── src/
│   │   │   └── main.cpp   # Firmware source
│   │   └── platformio.ini
│   └── src/               # Pico SDK project (legacy)
│       ├── main.c
│       ├── hal/
│       ├── flash/
│       ├── protocol/
│       └── usb/
├── host/                  # Host tools
│   └── cli/              # Python CLI
├── chipdb/               # Chip databases
├── docs/                 # Documentation & Web UI
│   ├── index.html        # Web interface
│   └── *.uf2            # Pre-built firmware
├── tests/               # Test files
├── openocd/             # OpenOCD config
├── LICENSE              # GPLv3
├── README.md           # This file
└── CHANGELOG.md       # Version history
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [README.md](README.md) | This file |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
| [docs/index.html](docs/index.html) | Web interface |
| [LICENSE](LICENSE) | GPLv3 License |

---

## Troubleshooting

### Web Interface Not Connecting

1. Ensure FiXPro firmware is flashed
2. Check USB cable supports data transfer
3. Try a different USB port
4. Close other applications using the serial port

### Device Not Detected

```bash
# Linux: Check permissions
ls -l /dev/ttyACM*
sudo chmod 666 /dev/ttyACM0

# Or add user to dialout group
sudo usermod -a -G dialout $USER
```

### PING Returns Nothing

1. Verify firmware is running (LED should blink)
2. Check baud rate is 115200
3. Ensure no terminal software is blocking the port

### SPI/I2C Communication Fails

1. Verify wiring connections
2. Check target device power
3. Ensure voltage levels are compatible (3.3V)

---

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

---

## License

This project is licensed under **GNU General Public License v3.0 (GPL-3.0)**.

See [LICENSE](LICENSE) for full text.

---

## Acknowledgments

- [Raspberry Pi Foundation](https://www.raspberrypi.org/) — RP2040 microcontroller
- [pico-sdk](https://github.com/raspberrypi/pico-sdk) — Official Pico SDK
- [TinyUSB](https://github.com/hathach/tinyusb) — USB stack
- [Earle Philhower](https://github.com/earlephilhower) — Arduino RP2040 core

---

<div align="center">

**FiXPro: Flash iX Pro — Universal. Fast. Open.**

[GitHub](https://github.com/MYMDO/fixpro) • [Releases](https://github.com/MYMDO/fixpro/releases) • [Issues](https://github.com/MYMDO/fixpro/issues)

</div>
