# FiXPro - Flash iX Pro

<div align="center">

![FiXPro Logo](docs/fixpro-logo.svg)

**Universal Hardware Programmer built on Raspberry Pi RP2040**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Build Status](https://github.com/MYMDO/fixpro/actions/workflows/build.yml/badge.svg)](https://github.com/MYMDO/fixpro/actions)
[![Release](https://img.shields.io/github/v/release/MYMDO/fixpro)](https://github.com/MYMDO/fixpro/releases)

</div>

---

## Overview

FiXPro is an open-source universal hardware programmer designed for SPI Flash and I2C EEPROM programming. Built on Raspberry Pi RP2040 with USB CDC interface, it provides reliable communication with target devices.

**FiXPro** stands for "Flash iX Pro" — universal flash programming with professional performance.

## Features

### Protocol Support
| Protocol | Max Speed | Status |
|----------|-----------|--------|
| SPI Flash | 50 MHz | ✅ Ready |
| I2C EEPROM | 1 MHz | ✅ Ready |
| JTAG | - | 🔄 Planned |
| SWD | - | 🔄 Planned |

### Hardware Platform
- **Processor**: Raspberry Pi RP2040 (Dual ARM Cortex-M0+ @ 133MHz)
- **USB**: USB 2.0 Full Speed via TinyUSB CDC
- **Interface**: Custom packet-based protocol over virtual COM port
- **Storage**: 2MB onboard Flash, 264KB SRAM

### Supported Chips
- **SPI Flash**: Winbond W25Qxx, Macronix MX25L, GigaDevice GD25Q, and more
- **I2C EEPROM**: 24Cxx series, and more

## Quick Start

### 1. Build Firmware

```bash
# Clone repository
git clone https://github.com/MYMDO/fixpro.git
cd fixpro

# Build firmware
cd firmware
mkdir -p build
cd build

# Configure (Linux/macOS)
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=../pico-sdk/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake \
  -DPICO_SDK_PATH=../pico-sdk \
  -DPICO_BOARD=pico \
  -DCMAKE_BUILD_TYPE=Release

# Or use system ARM toolchain
cmake -S . -B build \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DPICO_BOARD=pico \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)
```

### 2. Flash Device

1. Hold `BOOTSEL` button on Raspberry Pi Pico
2. Connect USB cable to computer
3. Release `BOOTSEL` button
4. Device will appear as mass storage
5. Copy `FiXPro.uf2` to the device

```bash
# Or use picotool (Linux)
picotool load -f FiXPro.uf2 -v
```

### 3. Install Host Software

```bash
# Install dependencies
pip install pyserial click

# Run CLI
python -m host.cli.fixpro --help
```

## Project Structure

```
FiXPro/
├── firmware/              # RP2040 Firmware (C/C++)
│   ├── src/
│   │   ├── main.c       # Entry point
│   │   ├── usb/         # USB CDC protocol
│   │   ├── hal/         # Hardware abstraction layer
│   │   ├── safety/      # Safety monitoring
│   │   └── flash/       # SPI Flash drivers
│   ├── pio/             # PIO state machines
│   └── CMakeLists.txt   # Build configuration
├── host/
│   └── cli/            # Python CLI tool
├── docs/               # Documentation
│   ├── BUILD.md        # Build instructions
│   ├── SPEC.md         # Technical specification
│   ├── USB_Protocol.md # Protocol documentation
│   └── Flash_Guide.md  # Programming guide
└── chipdb/             # Chip identification database
```

## Documentation

| Document | Description |
|----------|-------------|
| [BUILD.md](docs/BUILD.md) | Detailed build instructions for all platforms |
| [SPEC.md](docs/SPEC.md) | Technical specification and architecture |
| [USB_Protocol.md](docs/USB_Protocol.md) | USB communication protocol |
| [Flash_Guide.md](docs/Flash_Guide.md) | Flash programming guide |

## Usage Examples

### Detect Flash Chip

```bash
python -m host.cli.fixpro detect
```

### Read SPI Flash

```bash
python -m host.cli.fixpro read --output firmware.bin
```

### Write SPI Flash

```bash
python -m host.cli.fixpro write --input firmware.bin
```

### Verify Flash

```bash
python -m host.cli.fixpro verify --input firmware.bin
```

### Erase Flash

```bash
python -m host.cli.fixpro erase
```

## GitHub Actions

Firmware builds automatically on every push and release:

- **Push to main**: Creates build artifacts
- **Tag v*.*.***: Creates GitHub Release with firmware binaries

Latest release: https://github.com/MYMDO/fixpro/releases/latest

## License

This project is licensed under [GNU General Public License v3.0](LICENSE).

## Acknowledgments

- [Raspberry Pi Foundation](https://www.raspberrypi.org/) - RP2040 microcontroller
- [pico-sdk](https://github.com/raspberrypi/pico-sdk) - Official SDK
- [TinyUSB](https://github.com/hathach/tinyusb) - USB stack

---

**FiXPro: Flash iX Pro — Universal. Fast. Open.**
