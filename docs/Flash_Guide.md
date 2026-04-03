# Flash Programming Guide

This guide describes how to flash firmware onto devices using FiXPro.

## Prerequisites

### Hardware
- Raspberry Pi Pico with FiXPro firmware
- Target device (SPI Flash, I2C EEPROM)
- Jumper wires
- USB-C cable

### Software
- Python 3.8+
- pyserial library

## Wiring

### SPI Flash Connection

| Pico Pin | SPI Flash Pin | Notes |
|----------|---------------|-------|
| GP0 (MOSI) | DI (IO0) | Data input to flash |
| GP1 (MISO) | DO (IO1) | Data output from flash |
| GP2 (SCK) | CLK | Clock signal |
| GP3 | /CS | Chip select |
| 3V3 | VCC | Power (3.3V only!) |
| GND | GND | Ground |

**Warning:** Most Flash chips operate at 3.3V. Do NOT connect to 5V without level shifting.

### I2C EEPROM Connection

| Pico Pin | EEPROM Pin | Notes |
|----------|------------|-------|
| GP0 | SDA | Data line |
| GP2 | SCL | Clock line |
| 3V3 | VCC | Power (3.3V or 5V) |
| GND | GND | Ground |

## Programming Procedures

### 1. Detect Chip

Before programming, detect the connected chip:

```bash
python -m host.cli.fixpro detect
```

Expected output:
```
FiXPro v1.0.0
Detecting flash chip...
Chip found: Winbond W25Q128
Size: 16 MB (16777216 bytes)
```

### 2. Read Flash

Read the entire flash contents:

```bash
python -m host.cli.fixpro read --output flash_backup.bin
```

Read specific address range:

```bash
python -m host.cli.fixpro read --output flash_backup.bin --start 0x00000 --length 0x1000
```

### 3. Write Flash

Write data to flash:

```bash
python -m host.cli.fixpro write --input firmware.bin
```

Write with verification:

```bash
python -m host.cli.fixpro write --input firmware.bin --verify
```

### 4. Erase Flash

Erase entire chip:

```bash
python -m host.cli.fixpro erase
```

Erase specific sector:

```bash
python -m host.cli.fixpro erase --start 0x10000 --length 0x1000
```

### 5. Verify Flash

Compare flash contents with file:

```bash
python -m host.cli.fixpro verify --input firmware.bin
```

## Common Use Cases

### Backup BIOS Chip

```bash
# Read entire chip (8MB)
python -m host.cli.fixpro read --output bios_backup.bin --length 0x800000

# Verify backup
python -m host.cli.fixpro verify --input bios_backup.bin
```

### Program ESP32 Flash

```bash
# Read ESP32 firmware
python -m host.cli.fixpro read --output esp32_firmware.bin

# Write new firmware
python -m host.cli.fixpro erase
python -m host.cli.fixpro write --input new_firmware.bin --verify
```

### Repair Router Firmware

```bash
# Backup current firmware
python -m host.cli.fixpro detect
python -m host.cli.fixpro read --output router_backup.bin --length 0x400000

# Write stock firmware
python -m host.cli.fixpro erase
python -m host.cli.fixpro write --input stock_firmware.bin --verify
```

## Troubleshooting

### "Chip not detected"

1. Check wiring connections
2. Verify power supply (3.3V)
3. Check chip select connection
4. Ensure chip is not write-protected

### "Write failed"

1. Run erase before write
2. Check write protection status
3. Verify chip supports the operation

### "Verification failed"

1. Re-read the flash
2. Retry write operation
3. Check for hardware issues

### "Timeout error"

1. Reduce SPI frequency
2. Check wiring quality
3. Verify cable connections

## Safety Notes

1. **Always backup before writing** - Create a backup of existing contents first
2. **Verify after writing** - Always verify written data
3. **Check voltage** - Most flash chips are 3.3V only
4. **Handle with care** - ESD can damage chips
5. **Power off before connecting** - Connect wires with power off
