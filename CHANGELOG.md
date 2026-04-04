# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0] - 2026-04-04

### Added

#### Web Interface
- **FiXPro Hardware Control Panel** (`docs/index.html`)
  - Real-time device control buttons
  - PING, CAPS, GPIO, SPI_ID, I2C_SCAN commands
  - Connection status indicator
  - Activity log display
  - System monitor panel

- **Web Serial API Integration**
  - Full browser-to-device communication
  - No drivers required
  - Works on Windows, macOS, Linux

- **Chip Database**
  - 200+ supported chips
  - Real-time search and filtering
  - JEDEC ID auto-detection
  - Performance benchmarks

#### Firmware
- **PlatformIO Build System** (`firmware/platformio/`)
  - Arduino framework support
  - Earle Philhower RP2040 core
  - Simplified build process
  - USB CDC with TinyUSB

- **Text OPUP Protocol**
  - Human-readable commands
  - Line-based responses
  - Easy debugging

#### Documentation
- **Professional README.md**
  - Complete feature documentation
  - Quick start guide
  - Hardware requirements
  - Pin connections reference
  - Troubleshooting section

### Changed
- Firmware renamed to FiXPro v2.0.0
- Pre-built UF2: `FiXPro_platformio.uf2`
- Web interface now supports real device communication
- Updated chip database with 200+ entries

### Fixed
- USB descriptor compatibility
- Build system configuration
- String descriptor handling

## [2.0.0] - 2026-04-03

### Added

#### Firmware
- **UPDI (AVR) Driver** (`src/hal/updi.c`, `src/hal/updi.h`)
  - UART-based UPDI communication for AVR microcontrollers
  - Device info reading (signature, flash size, EEPROM size)
  - Flash read/write operations
  - EEPROM read/write operations
  - Fuse read/write operations
  - Chip erase functionality
  - Support for 115200/230400/460800 baud rates

- **1-Wire Driver** (`src/hal/onewire.c`, `src/hal/onewire.h`)
  - Bit-banged 1-Wire protocol implementation
  - Device search (ROM code discovery)
  - ROM code reading
  - DS18B20 temperature sensor support
  - CRC8 verification

- **USB Protocol Commands** (`src/usb/usb_protocol.c`, `src/usb/usb_protocol.h`)
  - `CMD_UPDI_INIT` (0x70) - Initialize UPDI interface
  - `CMD_UPDI_DEINIT` (0x71) - Deinitialize UPDI
  - `CMD_UPDI_RESET` (0x72) - Reset UPDI target
  - `CMD_UPDI_READ_INFO` (0x73) - Read device info
  - `CMD_UPDI_READ_FLASH` (0x74) - Read flash memory
  - `CMD_UPDI_WRITE_FLASH` (0x75) - Write flash memory
  - `CMD_1WIRE_INIT` (0x80) - Initialize 1-Wire
  - `CMD_1WIRE_RESET` (0x82) - Reset 1-Wire bus
  - `CMD_1WIRE_SEARCH` (0x83) - Search devices
  - `CMD_1WIRE_READ_TEMP` (0x85) - Read DS18B20 temperature

- **Capability Flags** in device info
  - `CAP_UPDI` (0x20) - AVR UPDI support
  - `CAP_1WIRE` (0x40) - 1-Wire support
  - `CAP_ISP` (0x10) - In-system programming

#### CLI
- **UPDI Commands**
  - `updi-info` - Read device info and identify chip
  - `updi-erase` - Erase target chip
  - `updi-read-fuse` - Read fuse value
  - `updi-write-fuse` - Write fuse value

- **1-Wire Commands**
  - `1wire-scan` - Scan for 1-Wire devices
  - `1wire-temp` - Read DS18B20 temperature

#### Database
- **AVR UPDI Chip Database** (`chipdb/avr.json`)
  - 54 supported AVR devices
  - ATtiny series (2KB-16KB Flash)
  - ATmega series (8KB-256KB Flash)
  - AVR-DA series (32KB-128KB Flash)
  - Signature-based chip identification

## [1.0.0] - 2026-03-XX

### Added
- SPI Flash programming (97+ chips)
- I2C EEPROM programming (61+ chips)
- JTAG debugging interface (25 MHz)
- SWD debugging interface (25 MHz)
- MCU database with 59 targets
- USB CDC protocol
- Safety monitoring system
- Raspberry Pi Pico RP2040 support
- GitHub Actions CI/CD

---

[2.1.0]: https://github.com/MYMDO/fixpro/releases/tag/v2.1.0
[2.0.0]: https://github.com/MYMDO/fixpro/releases/tag/v2.0.0
[1.0.0]: https://github.com/MYMDO/fixpro/releases/tag/v1.0.0
