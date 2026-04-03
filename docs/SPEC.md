# FiXPro Technical Specification

## Overview

FiXPro is a universal hardware programmer built on Raspberry Pi RP2040. It communicates with a host computer via USB CDC (Virtual COM Port) using a custom packet-based protocol.

## Hardware Platform

### Raspberry Pi Pico
| Feature | Specification |
|---------|---------------|
| MCU | RP2040 (Dual-core ARM Cortex-M0+) |
| Clock | 133 MHz |
| SRAM | 264 KB |
| Flash | 2 MB (onboard) |
| USB | USB 2.0 Full Speed |
| GPIO | 26 pins available |

### Target Interface
| Pin | Function | Notes |
|-----|----------|-------|
| GP0 | SPI MOSI / I2C SDA | Configurable |
| GP1 | SPI MISO | Configurable |
| GP2 | SPI SCK / I2C SCL | Configurable |
| GP3 | SPI CS | Configurable |
| GP6 | JTAG TCK / SWD CLK | Configurable |
| GP7 | JTAG TMS | Configurable |
| GP8 | JTAG TDI | Configurable |
| GP9 | JTAG TDO | Configurable |
| GP10 | SWD CLK | Configurable |
| GP11 | SWD DIO | Configurable |
| GP12 | UPDI | AVR programming |
| GP13 | 1-Wire | DS18B20 temperature |

## USB Communication

### Connection
- **Type**: USB 2.0 Full Speed
- **Interface**: CDC ACM (Virtual COM Port)
- **Baud Rate**: 115200 (default, not used for data)
- **VID**: 0x2E8A (Raspberry Pi)
- **PID**: 0x000A

### Packet Format

All communication uses binary packets with the following structure:

```
+--------+--------+--------+----------+
| SYNC   | CMD    | LENGTH        | DATA    |
+--------+--------+--------+----------+
| 1 byte | 1 byte | 2 bytes (LE) | N bytes |
+--------+--------+--------+----------+
```

#### SYNC Byte
- Value: `0xAB`
- Marks the start of a packet

#### CMD Byte
- Command code (host → device): `0x01` - `0x7F`
- Status code (device → host): `0x80` - `0xFF`

#### LENGTH
- 16-bit unsigned integer, little-endian
- Length of DATA field (0-4096 bytes)

## Command Reference

### System Commands

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x01 | PING | Connection test | STAT_OK |
| 0x02 | GET_INFO | Device information | STAT_OK_WITH_DATA |
| 0x03 | RESET | Reboot device | STAT_OK |
| 0x04 | GET_STATUS | Device status | STAT_OK_WITH_DATA |

### SPI Commands

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x10 | SPI_INIT | Initialize SPI | STAT_OK / STAT_ERROR |
| 0x11 | SPI_DEINIT | Deinitialize SPI | STAT_OK |
| 0x12 | SPI_TRANSFER | Transfer data | STAT_OK_WITH_DATA |
| 0x13 | SPI_WRITE | Write data | STAT_OK |
| 0x14 | SPI_READ | Read data | STAT_OK_WITH_DATA |

### I2C Commands

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x20 | I2C_INIT | Initialize I2C | STAT_OK / STAT_ERROR |
| 0x21 | I2C_DEINIT | Deinitialize I2C | STAT_OK |
| 0x22 | I2C_WRITE | Write data | STAT_OK / STAT_ERROR_NACK |
| 0x23 | I2C_READ | Read data | STAT_OK_WITH_DATA |
| 0x24 | I2C_SCAN | Scan I2C bus | STAT_OK_WITH_DATA |

### JTAG Commands

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x30 | JTAG_INIT | Initialize JTAG | STAT_OK / STAT_ERROR |
| 0x31 | JTAG_DEINIT | Deinitialize JTAG | STAT_OK |
| 0x32 | JTAG_RESET | Reset JTAG chain | STAT_OK |
| 0x33 | JTAG_SHIFT | Shift data | STAT_OK_WITH_DATA |
| 0x34 | JTAG_READ_IDCODE | Read IDCODE | STAT_OK_WITH_DATA |

### SWD Commands

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x40 | SWD_INIT | Initialize SWD | STAT_OK / STAT_ERROR |
| 0x41 | SWD_DEINIT | Deinitialize SWD | STAT_OK |
| 0x42 | SWD_RESET | Reset SWD line | STAT_OK |
| 0x43 | SWD_READ | Read register | STAT_OK_WITH_DATA |
| 0x44 | SWD_WRITE | Write register | STAT_OK / STAT_ERROR |

### UPDI Commands (AVR Programming)

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x70 | UPDI_INIT | Initialize UPDI | STAT_OK / STAT_ERROR |
| 0x71 | UPDI_DEINIT | Deinitialize UPDI | STAT_OK |
| 0x72 | UPDI_RESET | Reset UPDI | STAT_OK / STAT_ERROR |
| 0x73 | UPDI_READ_INFO | Read device info | STAT_OK_WITH_DATA |
| 0x74 | UPDI_READ_FLASH | Read flash | STAT_OK_WITH_DATA |
| 0x75 | UPDI_WRITE_FLASH | Write flash | STAT_OK / STAT_ERROR |
| 0x76 | UPDI_READ_EEPROM | Read EEPROM | STAT_OK_WITH_DATA |
| 0x77 | UPDI_WRITE_EEPROM | Write EEPROM | STAT_OK / STAT_ERROR |
| 0x78 | UPDI_READ_FUSE | Read fuse | STAT_OK_WITH_DATA |
| 0x79 | UPDI_WRITE_FUSE | Write fuse | STAT_OK / STAT_ERROR |
| 0x7A | UPDI_ERASE_CHIP | Chip erase | STAT_OK / STAT_ERROR |

### 1-Wire Commands

| CMD | Name | Description | Response |
|-----|------|-------------|----------|
| 0x80 | 1WIRE_INIT | Initialize 1-Wire | STAT_OK / STAT_ERROR |
| 0x81 | 1WIRE_DEINIT | Deinitialize 1-Wire | STAT_OK |
| 0x82 | 1WIRE_RESET | Reset pulse | STAT_OK / STAT_ERROR_NO_DEVICE |
| 0x83 | 1WIRE_SEARCH | Search devices | STAT_OK_WITH_DATA |
| 0x84 | 1WIRE_READ_ROM | Read ROM | STAT_OK_WITH_DATA |
| 0x85 | 1WIRE_READ_TEMP | Read DS18B20 temp | STAT_OK_WITH_DATA |

## Status Codes

| Code | Name | Description |
|------|------|-------------|
| 0x80 | STAT_OK | Success |
| 0x81 | STAT_OK_WITH_DATA | Success with response data |
| 0xE0 | STAT_ERROR | Generic error |
| 0xE1 | STAT_ERROR_PARAM | Invalid parameters |
| 0xE2 | STAT_ERROR_TIMEOUT | Operation timeout |
| 0xE3 | STAT_ERROR_BUS | Bus error |
| 0xE4 | STAT_ERROR_NACK | I2C NACK received |

## Device Information

Device responds to `GET_INFO` (0x02) with:

```c
struct device_info {
    uint8_t  version_major;     // Firmware major version
    uint8_t  version_minor;     // Firmware minor version
    uint8_t  version_patch;     // Firmware patch version
    uint32_t capabilities;      // Supported features bitfield
    uint8_t  hw_revision;      // Hardware revision
    char     serial[12];        // Serial number
    uint32_t flash_size;        // On-chip flash size
    uint32_t sram_size;        // SRAM size
};
```

### Capability Flags

| Flag | Name | Description |
|------|------|-------------|
| BIT 0 | CAP_SPI | SPI support |
| BIT 1 | CAP_I2C | I2C support |
| BIT 2 | CAP_JTAG | JTAG support |
| BIT 3 | CAP_SWD | SWD support |
| BIT 4 | CAP_ISP | In-system programming |
| BIT 5 | CAP_UPDI | AVR UPDI support |
| BIT 6 | CAP_1WIRE | 1-Wire support |
| BIT 8 | CAP_FLASH_READ | Flash read support |
| BIT 9 | CAP_FLASH_WRITE | Flash write support |
| BIT 15 | CAP_SAFETY | Safety monitoring |

## SPI Configuration

```c
struct spi_config {
    uint32_t frequency;      // Clock frequency in Hz
    uint8_t  mode;          // SPI mode (0-3)
    uint8_t  bits_per_word; // Bits per word (8 or 16)
    uint8_t  cs_pin;        // Chip select pin
};
```

### SPI Modes

| Mode | CPOL | CPHA | Description |
|------|------|------|-------------|
| 0 | 0 | 0 | Clock idle low, sample on rising |
| 1 | 0 | 1 | Clock idle low, sample on falling |
| 2 | 1 | 0 | Clock idle high, sample on falling |
| 3 | 1 | 1 | Clock idle high, sample on rising |

## I2C Configuration

```c
struct i2c_config {
    uint32_t frequency;      // Clock frequency in Hz
    uint8_t  address;       // Device address (7-bit)
};
```

## Flash Operations

### Chip Detection
FiXPro supports JEDEC ID detection for automatic chip identification.

### Supported Flash Chips

#### Winbond W25Qxx
- W25Q80 (1 MB)
- W25Q16 (2 MB)
- W25Q32 (4 MB)
- W25Q64 (8 MB)
- W25Q128 (16 MB)
- W25Q256 (32 MB)

#### Macronix MX25L
- MX25L1606 (2 MB)
- MX25L3206 (4 MB)
- MX25L6406 (8 MB)
- MX25L128 (16 MB)
- MX25L256 (32 MB)

### Flash Commands
| Command | OpCode | Description |
|---------|--------|-------------|
| READ | 0x03 | Read data (up to 50 MHz) |
| FAST_READ | 0x0B | Fast read (up to 50 MHz) |
| WRITE_ENABLE | 0x06 | Enable writing |
| WRITE_DISABLE | 0x04 | Disable writing |
| SECTOR_ERASE | 0x20 | Erase 4KB sector |
| BLOCK_ERASE_32K | 0x52 | Erase 32KB block |
| BLOCK_ERASE_64K | 0xD8 | Erase 64KB block |
| CHIP_ERASE | 0xC7 | Erase entire chip |
| PAGE_PROGRAM | 0x02 | Program 256-byte page |
| READ_JEDEC_ID | 0x9F | Read JEDEC ID |
| READ_STATUS | 0x05 | Read status register |
| WRITE_STATUS | 0x01 | Write status register |

## File Structure

### Firmware Source
```
firmware/
├── src/
│   ├── main.c           # Entry point, main loop
│   ├── usb/
│   │   └── usb_protocol.c  # USB CDC + packet handling
│   ├── hal/
│   │   ├── hal.c       # Hardware abstraction
│   │   ├── hal.h       # HAL interface
│   │   ├── jtag.c      # JTAG driver
│   │   ├── swd.c       # SWD driver
│   │   ├── updi.c       # UPDI driver
│   │   └── onewire.c   # 1-Wire driver
│   ├── safety/
│   │   └── safety.c    # Safety monitoring
│   └── flash/
│       └── spi_flash.c # SPI Flash operations
└── pio/
    ├── spi.pio         # SPI PIO state machine
    ├── i2c.pio         # I2C PIO state machine
    ├── jtag.pio        # JTAG PIO state machine
    └── swd.pio         # SWD PIO state machine
```

## Timing

### USB Packet Timeout
- Default: 1000 ms
- Configurable per command

### SPI Timing
- Setup time: 10 ns minimum
- Hold time: 10 ns minimum
- Clock frequency: Up to 50 MHz

### I2C Timing
- Standard mode: 100 kHz
- Fast mode: 400 kHz
- Fast mode+: 1 MHz

### JTAG Timing
- Clock frequency: Up to 25 MHz (software bit-bang)
- Setup time: 10 ns minimum
- Hold time: 10 ns minimum

### SWD Timing
- Clock frequency: Up to 25 MHz
- Turnaround time: Configurable
- Line reset: 50+ idle clocks

### UPDI Timing
- UART baud rate: 115200 / 230400 / 460800
- SYNCH character: 0x55
- Character frame: 8N1

### 1-Wire Timing
- Reset pulse: 480 µs
- Read/Write slot: 60-120 µs
- Conversion time (DS18B20): 750 ms

## Error Handling

All operations return status codes. Host software should:
1. Check response status before processing data
2. Implement timeout handling
3. Retry failed operations with backoff
4. Log errors for debugging
