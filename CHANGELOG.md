# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0] - 2026-04-06

### Added

#### Firmware (PlatformIO)
- **Full SPI Flash Operations**
  - `SPI_READ <addr> <len>` - Read data from SPI flash
  - `SPI_WRITE <addr> <hexdata>` - Write data to SPI flash
  - `SPI_ERASE <addr> [len]` - Erase SPI sector (default 4KB)
  - `SPI_ERASE_CHIP` - Full chip erase
  - `SPI_ID` - Read JEDEC ID

- **Full I2C Operations**
  - `I2C_READ <addr> <reg> [len]` - Read from I2C register
  - `I2C_WRITE <addr> <reg> <data>` - Write to I2C register
  - `I2C_SCAN` - Scan I2C bus for devices

- **GPIO Operations**
  - `GPIO` - Read GPIO states (GP10-GP17)
  - `GPIO_SET` - Set LED on (GP25)
  - `GPIO_CLR` - Set LED off

- **System Commands**
  - `STATUS` - System status with version, GPIO, capabilities
  - `INFO` - Detailed device information
  - `HELP` - Command reference

#### Web Interface
- **Modular JavaScript Architecture** (`docs/assets/fixpro-core.js`)
  - Logger module with DEBUG/INFO/WARN/ERROR levels
  - Serial module for Web Serial API communication
  - Events module for pub/sub communication
  - Protocol module with all command implementations
  - UI module for connection status updates
  - Actions module for high-level operations

- **FiXPro Hardware Control Panel**
  - PING, CAPS, GPIO, SPI_ID, I2C_SCAN commands
  - Connection status indicator
  - Activity log display

#### Documentation
- Complete protocol reference
- Pin configuration reference
- Troubleshooting section

### Changed
- Firmware version: 2.1.0
- Web interface version: 2.1.0
- pyproject.toml version: 2.1.0
- build.yml release tag: v2.1.0
- README updated with correct CAPS response format
- CLI imports fixed (removed duplicate try/except blocks)
- Makefile test-cli target fixed

### Fixed
- Version consistency across all files
- Duplicate log entries (removed extra event handler)
- License reference in README (GPL-3.0 vs MIT)
- Project structure in README (removed openocd/)
- Duplicate `tests/` directory entry in README
- pyproject.toml entry point (`fixpro.cli.main:main`)
- Makefile CLI test path

### Refactored
- `config.h` - Added board info, architecture, max frequencies
- `commands.cpp` - Added full SPI_WRITE, improved error handling
- `fixpro-core.js` - Complete rewrite with all commands
- `.github/workflows/build.yml` - Improved release naming
- `.github/workflows/quality.yml` - Added JS lint, HTML validation

## [2.0.0] - 2026-04-03

### Added

#### Firmware
- **UPDI (AVR) Driver**
  - UART-based UPDI communication for AVR microcontrollers
  - Device info reading, Flash read/write, EEPROM operations
  - Fuse read/write operations
  - Chip erase functionality

- **1-Wire Driver**
  - Bit-banged 1-Wire protocol implementation
  - Device search, DS18B20 temperature sensor support

- **USB Protocol Commands**
  - UPDI commands (0x70-0x7F)
  - 1-Wire commands (0x80-0x8F)

#### CLI
- UPDI commands: `updi-info`, `updi-erase`, `updi-read-fuse`, `updi-write-fuse`
- 1-Wire commands: `1wire-scan`, `1wire-temp`

#### Database
- AVR UPDI Chip Database (`chipdb/avr.json`) - 54 devices

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
