# FiXPro - Flash iX Pro

<div align="center">

**Universal Hardware Programmer built on Raspberry Pi RP2040**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Build Status](https://github.com/MYMDO/fixpro/actions/workflows/build.yml/badge.svg)](https://github.com/MYMDO/fixpro/actions)
[![Release](https://img.shields.io/github/v/release/MYMDO/fixpro)](https://github.com/MYMDO/fixpro/releases)

</div>

---

## Overview

FiXPro is an open-source universal hardware programmer designed for SPI Flash, I2C EEPROM programming, and ARM MCU debugging via JTAG/SWD. Built on Raspberry Pi RP2040 with USB CDC interface.

**FiXPro** stands for "Flash iX Pro" — universal flash programming with professional performance.

## Features

### Protocol Support
| Protocol | Max Speed | Status |
|----------|-----------|--------|
| SPI Flash | 80 MHz | ✅ Ready |
| I2C EEPROM | 1 MHz | ✅ Ready |
| JTAG | 25 MHz | ✅ Ready |
| SWD | 25 MHz | ✅ Ready |
| UPDI (AVR) | 460.8 kHz | ✅ Ready |
| 1-Wire (DS18B20) | - | ✅ Ready |

### Supported Chips
- **SPI Flash**: Winbond W25Qxx, Macronix MX25L, GigaDevice GD25Q, Micron, ISSI, ESMT, and more (97+ chips)
- **I2C EEPROM**: 24Cxx series, AT24Cxx, M24Cxx, FM24Cxx, and more (61+ chips)
- **MCU Targets**: STM32, Nordic nRF52, NXP LPC/Kinetis, Espressif ESP32, Atmel SAMD/AVR (59+ targets)

### Hardware Platform
- **Processor**: Raspberry Pi RP2040 (Dual ARM Cortex-M0+ @ 133MHz)
- **USB**: USB 2.0 Full Speed via TinyUSB CDC
- **Storage**: 2MB onboard Flash, 264KB SRAM

## Quick Start

### 1. Build Firmware

```bash
# Clone repository
git clone https://github.com/MYMDO/fixpro.git
cd fixpro

# Build firmware
cd firmware
mkdir -p build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico
cmake --build . -j$(nproc)
```

### 2. Flash Device

1. Hold `BOOTSEL` button on Raspberry Pi Pico
2. Connect USB cable
3. Release `BOOTSEL` button
4. Copy `FiXPro.uf2` to the device

### 3. Install CLI Tool

```bash
# Install from source
cd host
pip install -e .

# Or use directly
python -m fixpro.cli --help
```

## Usage

### Flash Operations

```bash
# Detect flash chip
fixpro detect

# Read flash
fixpro read -o firmware.bin

# Write flash
fixpro write -i firmware.bin --verify

# Erase sector
fixpro erase --sector 0x1000

# Erase entire chip
fixpro erase --chip
```

### I2C Operations

```bash
# Scan I2C bus
fixpro i2c-scan
```

### Debug Interface

```bash
# Read JTAG IDCODE
fixpro jtag-idcode

# Connect to ARM DAP
fixpro swd-dap
```

### UPDI (AVR Programming)

```bash
# Initialize UPDI
fixpro updi-init --baud 115200

# Read device info
fixpro updi-info

# Erase chip
fixpro updi-erase

# Read flash
fixpro updi-read-flash --address 0 --length 256

# Read fuses
fixpro updi-read-fuse --fuse 0
```

### 1-Wire (DS18B20)

```bash
# Scan for 1-Wire devices
fixpro 1wire-scan

# Read temperature from DS18B20
fixpro 1wire-temp
```

### Information

```bash
# Device info
fixpro info

# List supported chips
fixpro list-chips --type spi
fixpro list-chips --type mcu
```

## Project Structure

```
FiXPro/
├── firmware/              # RP2040 Firmware
│   ├── src/
│   │   ├── main.c       # Entry point
│   │   ├── fixpro_errors.h  # Error codes
│   │   ├── usb/         # USB CDC protocol
│   │   ├── hal/         # Hardware abstraction
│   │   │   ├── hal.c/h  # Core HAL
│   │   │   ├── jtag.c/h # JTAG driver
│   │   │   ├── swd.c/h  # SWD driver
│   │   │   ├── updi.c/h # UPDI (AVR) driver
│   │   │   └── onewire.c/h # 1-Wire driver
│   │   ├── safety/      # Safety monitoring
│   │   └── flash/       # SPI Flash drivers
│   ├── pio/             # PIO state machines
│   └── CMakeLists.txt
├── host/
│   ├── cli/             # Python CLI
│   │   ├── __init__.py
│   │   ├── main.py      # CLI entry point
│   │   ├── device.py    # Device communication
│   │   └── protocols/    # Protocol definitions
│   └── pyproject.toml
├── chipdb/              # Chip databases
│   ├── spi_flash.json   # 97+ SPI Flash chips
│   ├── i2c_eeprom.json # 61+ EEPROM chips
│   └── mcu.json        # 59+ MCU targets
└── docs/               # Documentation
```

## Documentation

| Document | Description |
|----------|-------------|
| [BUILD.md](docs/BUILD.md) | Build instructions |
| [SPEC.md](docs/SPEC.md) | Technical specification |
| [USB_Protocol.md](docs/USB_Protocol.md) | USB protocol reference |
| [Flash_Guide.md](docs/Flash_Guide.md) | Flash programming guide |

## GitHub Actions

Firmware builds automatically on every push and release:

- **Push to main**: Creates build artifacts
- **Tag v*.*.***: Creates GitHub Release

Latest release: https://github.com/MYMDO/fixpro/releases/latest

## License

This project is licensed under [GNU General Public License v3.0](LICENSE).

## Acknowledgments

- [Raspberry Pi Foundation](https://www.raspberrypi.org/) - RP2040 microcontroller
- [pico-sdk](https://github.com/raspberrypi/pico-sdk) - Official SDK
- [TinyUSB](https://github.com/hathach/tinyusb) - USB stack

---

**FiXPro: Flash iX Pro — Universal. Fast. Open.**
