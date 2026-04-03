# FiXPro - Flash iX Pro

<div align="center">

![FiXPro Logo](docs/fixpro-logo.svg)

**Next-generation universal hardware programmer built on Raspberry Pi RP2040**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-latest-brightgreen)](docs/)

</div>

---

## Overview

FiXPro is an open-source, high-performance universal hardware programmer designed to replace and surpass budget programmers like CH341A, USBasp, and ST-Link v2. Built on the powerful Raspberry Pi RP2040 microcontroller with programmable I/O (PIO) state machines, FiXPro delivers unprecedented flexibility and speed.

**FiXPro** stands for "Flash iX Pro" — where "iX" represents the universal nature (I²C, SPI, etc.) and "Pro" denotes professional-grade performance.

## Key Features

### Protocol Support (Core)
| Protocol | Speed | Status |
|----------|-------|--------|
| SPI Flash | up to 50MHz | ✅ Ready |
| I2C EEPROM | up to 1MHz | ✅ Ready |
| JTAG | Adaptive | 🔄 Planned |
| SWD | up to 50MHz | 🔄 Planned |

### Architecture Highlights
- **RP2040 Dual-Core**: 133MHz ARM Cortex-M0+ processors
- **8 PIO State Machines**: Hardware-accelerated protocol emulation
- **USB 2.0 Full Speed**: High-speed communication with host
- **Hardware Safety**: Multiple protection layers for target devices
- **Open Source**: 100% open hardware and software

### Safety Features
```
├── Over-voltage protection (TVS diodes)
├── Over-current protection (PTC fuses)
├── ESD protection on all I/O lines
├── Reverse polarity protection
├── Real-time voltage monitoring
└── Thermal shutdown
```

## Quick Start

### Hardware Requirements
- Raspberry Pi Pico or compatible RP2040 board
- USB-C cable
- Target device (SPI Flash, I2C EEPROM)

### Software Requirements
- CMake 3.13+
- Pico SDK 1.5+
- Python 3.8+ (for CLI)

### Building Firmware
```bash
cd firmware
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Flashing
```bash
cp FiXPro.uf2 /mount-point-of-pico
```

### Using CLI
```bash
# Detect connected chip
python -m fixpro detect

# Read flash
python -m fixpro read --device W25Q128 --output firmware.bin

# Write flash
python -m fixpro write --device W25Q128 --input firmware.bin

# Verify
python -m fixpro verify --device W25Q128 --input firmware.bin
```

## Project Structure

```
FiXPro/
├── firmware/           # RP2040 firmware (C/C++)
│   ├── src/           # Main source code
│   │   ├── main.c    # Entry point
│   │   ├── usb/      # USB device stack
│   │   ├── protocol/ # Protocol implementations
│   │   ├── hal/      # Hardware abstraction
│   │   ├── safety/   # Safety monitoring
│   │   └── flash/    # Flash memory drivers
│   └── pio/          # PIO state machines (.pio files)
├── host/              # Host computer software
│   ├── cli/          # Command-line tool
│   ├── lib/          # Shared library
│   └── python/       # Python bindings
├── chipdb/           # Chip identification database
├── openocd/          # OpenOCD config files
├── docs/             # Documentation
└── tests/            # Test suites
```

## Performance Comparison

| Programmer | SPI Speed | Protocols | Safety | Price |
|------------|-----------|-----------|--------|-------|
| CH341A | 2 MHz | 2 | ❌ | $2-3 |
| USBasp | N/A | 1 (ISP) | ⚠️ | $2-4 |
| ST-Link v2 | N/A | 1 (SWD) | ⚠️ | $3-5 |
| **FiXPro** | **50 MHz** | **12+** | **✅** | **$4-5** |

## Contributing

Contributions are welcome! Please read our [Contributing Guide](docs/CONTRIBUTING.md) before submitting pull requests.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Raspberry Pi Foundation for the amazing RP2040 chip
- pico-sdk contributors
- Open-source hardware community

---

**FiXPro: Flash iX Pro — Universal. Fast. Safe. Open.**
