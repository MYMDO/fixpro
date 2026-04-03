# USB Protocol Documentation

FiXPro uses a custom binary protocol over USB CDC (Virtual COM Port).

## Overview

- **Transport**: USB CDC ACM (Virtual COM Port)
- **Baud Rate**: 115200 (informational, not used for timing)
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Packet Type**: Binary (not ASCII text)

## Connection

### Finding the Device

FiXPro appears as a virtual COM port:
- **Windows**: COM3, COM4, etc. (check Device Manager)
- **Linux**: /dev/ttyACM0, /dev/ttyACM1, etc.
- **macOS**: /dev/cu.usbmodemXXXX

### Connection Parameters

```python
import serial

ser = serial.Serial(
    port='/dev/ttyACM0',
    baudrate=115200,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1.0
)
```

## Packet Structure

All packets have the following binary structure:

```
+--------+------+-----------------+---------+
| SYNC   | CMD  | LENGTH          | PAYLOAD|
+--------+------+-----------------+---------+
| 1 byte | 1    | 2 bytes (LE)   | N bytes |
+--------+------+-----------------+---------+
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| SYNC | uint8 | Synchronization byte (0xAB) |
| CMD | uint8 | Command or status code |
| LENGTH | uint16_le | Payload length (0-4096) |
| PAYLOAD | bytes | Command-specific data |

## Command Format

### Host → Device

```python
def send_command(cmd, payload=b''):
    packet = bytes([0xAB, cmd])          # SYNC + CMD
    packet += len(payload).to_bytes(2, 'little')  # LENGTH
    packet += payload                      # PAYLOAD
    ser.write(packet)
```

### Device → Host

Response packets have the same structure, but:
- CMD field contains status code (see Status Codes)
- PAYLOAD contains response data (if any)

## Command Reference

### System Commands

#### PING (0x01)
Test communication.

**Request**: No payload
```python
send_command(0x01)
```

**Response**:
```
CMD: STAT_OK (0x80)
Payload: None
```

#### GET_INFO (0x02)
Get device information.

**Request**: No payload
```python
send_command(0x02)
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: device_info_t (16 bytes)
```

**Payload Structure**:
```c
struct device_info {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint8_t  version_patch;
    uint32_t capabilities;     // Capability flags
    uint8_t  hw_revision;
    char     serial[12];
    uint32_t flash_size;
    uint32_t sram_size;
};
```

#### RESET (0x03)
Reboot device into bootloader mode.

**Request**: No payload
```python
send_command(0x03)
```

**Response**:
```
CMD: STAT_OK (0x80)
```

Device will reboot.

### SPI Commands

#### SPI_INIT (0x10)
Initialize SPI interface.

**Request**:
```c
struct spi_config {
    uint32_t frequency;    // Hz (e.g., 10000000 for 10MHz)
    uint8_t  mode;       // SPI mode 0-3
    uint8_t  bits;       // Bits per word (8 or 16)
    uint8_t  cs_pin;     // Chip select GPIO
};
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### SPI_TRANSFER (0x12)
Transfer data on SPI bus.

**Request**:
```
+------+
| data |  // Bytes to send
+------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: received bytes
```

#### SPI_READ (0x14)
Read data from SPI flash.

**Request**:
```
+----------+----------+
| address  | length   |
+----------+----------+
| 4 bytes  | 2 bytes  |
+----------+----------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: read bytes
```

### I2C Commands

#### I2C_INIT (0x20)
Initialize I2C interface.

**Request**:
```c
struct i2c_config {
    uint32_t frequency;    // Hz (e.g., 400000 for 400kHz)
    uint8_t  address;      // 7-bit device address
};
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### I2C_WRITE (0x22)
Write data to I2C device.

**Request**:
```
+-----+----------+
| addr| data    |
+-----+----------+
| 1   | N bytes |
+-----+----------+
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR_NACK (0xE4)
```

#### I2C_READ (0x23)
Read data from I2C device.

**Request**:
```
+-----+----------+
| addr| length  |
+-----+----------+
| 1   | 2 bytes |
+-----+----------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: read bytes
```

### JTAG Commands

#### JTAG_INIT (0x30)
Initialize JTAG interface.

**Request**: No payload
```python
send_command(0x30)
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### JTAG_RESET (0x32)
Reset JTAG chain (set TMS high for 5+ clock cycles).

**Request**: No payload
```python
send_command(0x32)
```

**Response**:
```
CMD: STAT_OK (0x80)
```

#### JTAG_SHIFT (0x33)
Shift data through JTAG chain.

**Request**:
```
+------------+----------+
| num_bits   | tdi_data|
+------------+----------+
| 2 bytes    | N bytes |
+------------+----------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: tdo_data (bytes read from TDO)
```

#### JTAG_READ_IDCODE (0x34)
Read IDCODE from JTAG device.

**Request**: No payload
```python
send_command(0x34)
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: 4 bytes (32-bit IDCODE)
```

### SWD Commands

#### SWD_INIT (0x40)
Initialize SWD (Serial Wire Debug) interface.

**Request**: No payload
```python
send_command(0x40)
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### SWD_RESET (0x42)
Reset SWD line (50+ clocks with SWDIO high).

**Request**: No payload
```python
send_command(0x42)
```

**Response**:
```
CMD: STAT_OK (0x80)
```

#### SWD_READ (0x43)
Read from SWD register.

**Request**:
```
+----------+
| address  |
+----------+
| 1 byte   |
+----------+
```
- `address[7:4]` = APnDP (0=DP, 1=AP)
- `address[3:0]` = Register address (0-15)

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: 4 bytes (32-bit register value)
```

#### SWD_WRITE (0x44)
Write to SWD register.

**Request**:
```
+----------+----------+
| address  | value    |
+----------+----------+
| 1 byte   | 4 bytes  |
+----------+----------+
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

### UPDI Commands (AVR Programming)

#### UPDI_INIT (0x70)
Initialize UPDI interface.

**Request**:
```
+--------+
| baud   |
+--------+
| 4 bytes|
+--------+
```
- `baud`: UART baud rate (default 115200)

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### UPDI_READ_INFO (0x73)
Read UPDI device information.

**Request**: No payload
```python
send_command(0x73)
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: updi_device_info_t (10 bytes)
```

#### UPDI_READ_FLASH (0x74)
Read from AVR flash memory.

**Request**:
```
+--------+----------+
| addr   | length   |
+--------+----------+
| 2 bytes| 2 bytes  |
+--------+----------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: read bytes
```

#### UPDI_WRITE_FLASH (0x75)
Write to AVR flash memory.

**Request**:
```
+--------+----------+
| addr   | data     |
+--------+----------+
| 2 bytes| N bytes  |
+--------+----------+
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### UPDI_READ_EEPROM (0x76)
Read from AVR EEPROM.

**Request**:
```
+--------+----------+
| addr   | length   |
+--------+----------+
| 2 bytes| 2 bytes  |
+--------+----------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: read bytes
```

#### UPDI_WRITE_EEPROM (0x77)
Write to AVR EEPROM.

**Request**:
```
+--------+----------+
| addr   | data     |
+--------+----------+
| 2 bytes| N bytes  |
+--------+----------+
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### UPDI_READ_FUSE (0x78)
Read fuse value.

**Request**:
```
+------+
| fuse |
+------+
| 1    |
+------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: 1 byte (fuse value)
```

#### UPDI_WRITE_FUSE (0x79)
Write fuse value.

**Request**:
```
+------+-------+
| fuse | value |
+------+-------+
| 1    | 1     |
+------+-------+
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### UPDI_ERASE_CHIP (0x7A)
Perform chip erase via UPDI.

**Request**: No payload
```python
send_command(0x7A)
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

### 1-Wire Commands

#### 1WIRE_INIT (0x80)
Initialize 1-Wire interface.

**Request**: No payload
```python
send_command(0x80)
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR (0xE0)
```

#### 1WIRE_RESET (0x82)
Send 1-Wire reset pulse and detect presence.

**Request**: No payload
```python
send_command(0x82)
```

**Response**:
```
CMD: STAT_OK (0x80) or STAT_ERROR_NO_DEVICE (0xE5)
```

#### 1WIRE_SEARCH (0x83)
Search for 1-Wire devices on the bus.

**Request**:
```
+--------+
| max    |
+--------+
| 1 byte |
+--------+
```
- `max`: Maximum number of devices to find

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: onewire_device_t array (8 bytes per device + valid flag)
```

#### 1WIRE_READ_ROM (0x84)
Read ROM code from single device (only when one device present).

**Request**: No payload
```python
send_command(0x84)
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: 8 bytes (ROM code)
```

#### 1WIRE_READ_TEMP (0x85)
Start DS18B20 conversion and read temperature.

**Request**:
```
+--------+
| ROM    |
+--------+
| 8 bytes| (optional, skip ROM if not provided)
+--------+
```

**Response**:
```
CMD: STAT_OK_WITH_DATA (0x81)
Payload: ds18b20_data_t
- float temperature (4 bytes)
- bool valid (1 byte)
- uint8_t scratchpad[9] (9 bytes)
```

## Status Codes

| Code | Name | Description |
|------|------|-------------|
| 0x80 | STAT_OK | Operation successful |
| 0x81 | STAT_OK_WITH_DATA | Success with response data |
| 0xE0 | STAT_ERROR | Generic error |
| 0xE1 | STAT_ERROR_PARAM | Invalid parameters |
| 0xE2 | STAT_ERROR_TIMEOUT | Operation timed out |
| 0xE3 | STAT_ERROR_BUS | Bus error |
| 0xE4 | STAT_ERROR_NACK | I2C NACK received |
| 0xE5 | STAT_ERROR_NO_DEVICE | No device found |
| 0xE6 | STAT_ERROR_VERIFY | Verification failed |
| 0xE7 | STAT_ERROR_SAFETY | Safety violation detected |
| 0xE8 | STAT_ERROR_BUSY | Device busy |

## Example: Python Client

```python
import serial
import struct

USB_SYNC = 0xAB
STAT_OK = 0x80
STAT_OK_WITH_DATA = 0x81

class FiXPro:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=1.0)
    
    def send_cmd(self, cmd, payload=b''):
        pkt = bytes([USB_SYNC, cmd])
        pkt += len(payload).to_bytes(2, 'little')
        pkt += payload
        self.ser.write(pkt)
        
        # Read response
        sync = self.ser.read(1)
        status = self.ser.read(1)
        length = int.from_bytes(self.ser.read(2), 'little')
        data = self.ser.read(length) if length > 0 else b''
        
        return ord(status), data
    
    def ping(self):
        status, _ = self.send_cmd(0x01)
        return status == STAT_OK
    
    def get_info(self):
        status, data = self.send_cmd(0x02)
        if status == STAT_OK_WITH_DATA and len(data) >= 16:
            info = struct.unpack('<BBB I B 12s II', data[:29])
            return {
                'version': f'{info[0]}.{info[1]}.{info[2]}',
                'capabilities': info[3],
                'serial': info[5].decode().rstrip('\x00'),
            }
        return None
    
    def close(self):
        self.ser.close()

# Usage
fxp = FiXPro('/dev/ttyACM0')
print(fxpro.get_info())
fxp.close()
```

## Error Handling

1. **Check status byte** before processing data
2. **Implement timeouts** (default 1 second)
3. **Retry logic** with exponential backoff for transient errors
4. **Log errors** with command and response for debugging

## Notes

- All multi-byte integers are **little-endian**
- Maximum payload size is **4096 bytes**
- SYNC byte (0xAB) marks packet start
- Device ignores garbage data until SYNC is received
