"""
FiXPro Device - Communication class for FiXPro device
"""

import time
from typing import Optional, List, Dict, Any, Tuple
from .protocols import (
    SYNC_BYTE,
    USB_MAX_PACKET_SIZE,
    CMD_PING,
    CMD_GET_INFO,
    CMD_RESET,
    CMD_GET_STATUS,
    CMD_SPI_INIT,
    CMD_SPI_DEINIT,
    CMD_SPI_TRANSFER,
    CMD_SPI_WRITE,
    CMD_SPI_READ,
    CMD_I2C_INIT,
    CMD_I2C_WRITE,
    CMD_I2C_READ,
    CMD_I2C_SCAN,
    CMD_JTAG_INIT,
    CMD_JTAG_DEINIT,
    CMD_JTAG_RESET,
    CMD_JTAG_SHIFT,
    CMD_JTAG_READ_IDCODE,
    CMD_SWD_INIT,
    CMD_SWD_DEINIT,
    CMD_SWD_RESET,
    CMD_SWD_READ,
    CMD_SWD_WRITE,
    CMD_UPDI_INIT,
    CMD_UPDI_DEINIT,
    CMD_UPDI_RESET,
    CMD_UPDI_READ_INFO,
    CMD_UPDI_READ_FLASH,
    CMD_UPDI_WRITE_FLASH,
    CMD_UPDI_ERASE_CHIP,
    CMD_UPDI_READ_FUSE,
    CMD_UPDI_WRITE_FUSE,
    CMD_1WIRE_INIT,
    CMD_1WIRE_DEINIT,
    CMD_1WIRE_RESET,
    CMD_1WIRE_SEARCH,
    CMD_1WIRE_READ_ROM,
    CMD_1WIRE_READ_TEMP,
    STAT_OK,
    STAT_OK_WITH_DATA,
    STAT_ERROR_TIMEOUT,
)


class FiXProDevice:
    """Communication class for FiXPro device"""

    def __init__(self, port: Optional[str] = None, baudrate: int = 115200, timeout: float = 1.0):
        import serial
        import serial.tools.list_ports

        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None
        self.connected = False

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    def find_device(self):
        """Find FiXPro device by VID/PID or port name"""
        import serial
        import serial.tools.list_ports

        ports = serial.tools.list_ports.comports()

        for p in ports:
            if "FiXPro" in p.description or "fixpro" in p.description.lower():
                return p.device
            if "Pico" in p.description or " rp2040" in p.description.lower():
                try:
                    test_port = serial.Serial(p.device, 115200, timeout=0.1)
                    test_port.close()
                    return p.device
                except:
                    pass

        if ports:
            return ports[0].device
        return None

    def connect(self) -> bool:
        """Connect to FiXPro device"""
        import serial

        if self.connected and self.serial and self.serial.is_open:
            return True

        port = self.port
        if port is None:
            port = self.find_device()
            if port is None:
                return False

        try:
            self.serial = serial.Serial(
                port=port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
            )
            self.connected = True
            self.port = port
            return True
        except serial.SerialException:
            return False

    def disconnect(self) -> None:
        """Disconnect from device"""
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.connected = False

    def _send_packet(self, cmd: int, data: bytes = b"") -> bool:
        """Send packet to device"""
        if not self.connected or not self.serial:
            return False

        if len(data) > USB_MAX_PACKET_SIZE:
            return False

        packet = bytes([SYNC_BYTE, cmd]) + len(data).to_bytes(2, "little") + data

        try:
            self.serial.write(packet)
            self.serial.flush()
            return True
        except Exception:
            return False

    def _recv_response(self, timeout: Optional[float] = None) -> Tuple[int, bytes]:
        """Receive response from device"""
        if not self.connected or not self.serial:
            return STAT_ERROR_TIMEOUT, b""

        old_timeout = self.serial.timeout
        if timeout is not None:
            self.serial.timeout = timeout

        try:
            while True:
                byte = self.serial.read(1)
                if byte == bytes([SYNC_BYTE]):
                    break
                if timeout and (
                    time.time() - self.serial._start_time
                    if hasattr(self.serial, "_start_time")
                    else True
                ):
                    if not byte:
                        self.serial.timeout = old_timeout
                        return STAT_ERROR_TIMEOUT, b""

            header = self.serial.read(3)
            if len(header) < 3:
                self.serial.timeout = old_timeout
                return STAT_ERROR_TIMEOUT, b""

            status = header[0]
            length = int.from_bytes(header[1:3], "little")

            data = b""
            remaining = length
            while remaining > 0:
                chunk = self.serial.read(min(remaining, 4096))
                if not chunk:
                    break
                data += chunk
                remaining -= len(chunk)

            self.serial.timeout = old_timeout
            return status, data

        except Exception:
            self.serial.timeout = old_timeout
            return STAT_ERROR_TIMEOUT, b""

    def _execute(self, cmd: int, data: bytes = b"", timeout: float = 1.0) -> Tuple[bool, bytes]:
        """Execute command and return response"""
        if not self._send_packet(cmd, data):
            return False, b""

        status, response = self._recv_response(timeout)
        return status == STAT_OK or status == STAT_OK_WITH_DATA, response

    def _execute_with_status(
        self, cmd: int, data: bytes = b"", timeout: float = 1.0
    ) -> Tuple[int, bytes]:
        """Execute command and return status + data"""
        if not self._send_packet(cmd, data):
            return STAT_ERROR_TIMEOUT, b""
        return self._recv_response(timeout)

    def ping(self) -> bool:
        """Ping device"""
        success, _ = self._execute(CMD_PING)
        return success

    def get_info(self) -> Optional[Dict[str, Any]]:
        """Get device information"""
        status, data = self._execute_with_status(CMD_GET_INFO)
        if status == STAT_OK_WITH_DATA and len(data) >= 28:
            return {
                "version": f"{data[0]}.{data[1]}.{data[2]}",
                "capabilities": int.from_bytes(data[3:7], "little"),
                "hw_revision": data[7],
                "serial": data[8:20].decode("ascii", errors="replace").rstrip("\x00"),
                "flash_size": int.from_bytes(data[20:24], "little"),
                "sram_size": int.from_bytes(data[24:28], "little"),
            }
        return None

    def get_status(self) -> Optional[Dict[str, Any]]:
        """Get device safety status"""
        status, data = self._execute_with_status(CMD_GET_STATUS)
        if status == STAT_OK_WITH_DATA and len(data) >= 6:
            return {
                "voltage_mv": int.from_bytes(data[0:2], "little"),
                "current_ma": int.from_bytes(data[2:4], "little"),
                "temperature_c": data[4],
                "status": data[5],
            }
        return None

    def reset(self) -> bool:
        """Reset device"""
        success, _ = self._execute(CMD_RESET, timeout=0.5)
        self.disconnect()
        return success

    def spi_init(self, frequency: int = 10000000, mode: int = 0) -> bool:
        """Initialize SPI"""
        config = frequency.to_bytes(4, "little") + bytes([mode, 0, 0])
        success, _ = self._execute(CMD_SPI_INIT, config)
        return success

    def spi_read(self, address: int, length: int, timeout: float = 10.0) -> Optional[bytes]:
        """Read from SPI flash"""
        cmd_data = address.to_bytes(4, "little") + length.to_bytes(2, "little")
        status, data = self._execute_with_status(CMD_SPI_READ, cmd_data, timeout=timeout)
        if status == STAT_OK_WITH_DATA:
            return data
        return None

    def spi_write(self, address: int, data: bytes, timeout: float = 10.0) -> bool:
        """Write to SPI flash"""
        cmd_data = address.to_bytes(4, "little") + data
        success, _ = self._execute(CMD_SPI_WRITE, cmd_data, timeout=timeout)
        return success

    def spi_transfer(self, data: bytes) -> Optional[bytes]:
        """Raw SPI transfer"""
        status, response = self._execute_with_status(CMD_SPI_TRANSFER, data)
        if status == STAT_OK_WITH_DATA:
            return response
        return None

    def i2c_init(self, frequency: int = 400000) -> bool:
        """Initialize I2C"""
        config = frequency.to_bytes(4, "little") + bytes([0])
        success, _ = self._execute(CMD_I2C_INIT, config)
        return success

    def i2c_write(self, address: int, data: bytes) -> bool:
        """Write to I2C device"""
        cmd_data = bytes([address]) + data
        success, _ = self._execute(CMD_I2C_WRITE, cmd_data)
        return success

    def i2c_read(self, address: int, length: int) -> Optional[bytes]:
        """Read from I2C device"""
        cmd_data = bytes([address]) + length.to_bytes(2, "little")
        status, data = self._execute_with_status(CMD_I2C_READ, cmd_data)
        if status == STAT_OK_WITH_DATA:
            return data
        return None

    def i2c_scan(self) -> List[int]:
        """Scan I2C bus for devices"""
        status, data = self._execute_with_status(CMD_I2C_SCAN)
        if status == STAT_OK_WITH_DATA and data:
            return list(data)
        return []

    def jtag_init(self) -> bool:
        """Initialize JTAG interface"""
        success, _ = self._execute(CMD_JTAG_INIT)
        return success

    def jtag_deinit(self) -> bool:
        """Deinitialize JTAG interface"""
        success, _ = self._execute(CMD_JTAG_DEINIT)
        return success

    def jtag_reset(self) -> bool:
        """Reset JTAG chain"""
        success, _ = self._execute(CMD_JTAG_RESET)
        return success

    def jtag_shift(self, tdi_data: bytes, num_bits: int) -> Optional[bytes]:
        """Shift data through JTAG chain"""
        cmd_data = num_bits.to_bytes(2, "little") + tdi_data
        status, data = self._execute_with_status(CMD_JTAG_SHIFT, cmd_data)
        if status == STAT_OK_WITH_DATA:
            return data
        return None

    def jtag_read_idcode(self) -> Optional[int]:
        """Read IDCODE from JTAG device"""
        status, data = self._execute_with_status(CMD_JTAG_READ_IDCODE)
        if status == STAT_OK_WITH_DATA and len(data) >= 4:
            return int.from_bytes(data[:4], "little")
        return None

    def swd_init(self) -> bool:
        """Initialize SWD interface"""
        success, _ = self._execute(CMD_SWD_INIT)
        return success

    def swd_deinit(self) -> bool:
        """Deinitialize SWD interface"""
        success, _ = self._execute(CMD_SWD_DEINIT)
        return success

    def swd_reset(self) -> bool:
        """Reset SWD interface"""
        success, _ = self._execute(CMD_SWD_RESET)
        return success

    def swd_read(self, address: int) -> Optional[int]:
        """Read from SWD register"""
        status, data = self._execute_with_status(CMD_SWD_READ, bytes([address]))
        if status == STAT_OK_WITH_DATA and len(data) >= 4:
            return int.from_bytes(data[:4], "little")
        return None

    def swd_write(self, address: int, value: int) -> bool:
        """Write to SWD register"""
        cmd_data = bytes([address]) + value.to_bytes(4, "little")
        success, _ = self._execute(CMD_SWD_WRITE, cmd_data)
        return success

    def updi_init(self, baud: int = 115200) -> bool:
        """Initialize UPDI interface"""
        cmd_data = baud.to_bytes(4, "little")
        success, _ = self._execute(CMD_UPDI_INIT, cmd_data)
        return success

    def updi_deinit(self) -> bool:
        """Deinitialize UPDI interface"""
        success, _ = self._execute(CMD_UPDI_DEINIT)
        return success

    def updi_reset(self) -> bool:
        """Reset UPDI target"""
        success, _ = self._execute(CMD_UPDI_RESET)
        return success

    def updi_read_info(self) -> Optional[Dict[str, Any]]:
        """Read UPDI device info"""
        status, data = self._execute_with_status(CMD_UPDI_READ_INFO)
        if status == STAT_OK_WITH_DATA and len(data) >= 8:
            return {
                "signature": [data[0], data[1], data[2]],
                "revision": data[3],
                "flash_size": int.from_bytes(data[4:6], "little"),
                "eeprom_size": int.from_bytes(data[6:8], "little"),
            }
        return None

    def updi_read_flash(self, address: int, length: int) -> Optional[bytes]:
        """Read from UPDI target flash"""
        cmd_data = address.to_bytes(2, "little") + length.to_bytes(2, "little")
        status, data = self._execute_with_status(CMD_UPDI_READ_FLASH, cmd_data)
        if status == STAT_OK_WITH_DATA:
            return data
        return None

    def updi_write_flash(self, address: int, data: bytes) -> bool:
        """Write to UPDI target flash"""
        cmd_data = address.to_bytes(2, "little") + data
        success, _ = self._execute(CMD_UPDI_WRITE_FLASH, cmd_data)
        return success

    def updi_erase_chip(self) -> bool:
        """Erase UPDI target chip"""
        success, _ = self._execute(CMD_UPDI_ERASE_CHIP)
        return success

    def updi_read_fuse(self, fuse: int) -> Optional[int]:
        """Read UPDI fuse"""
        status, data = self._execute_with_status(CMD_UPDI_READ_FUSE, bytes([fuse]))
        if status == STAT_OK_WITH_DATA and data:
            return data[0]
        return None

    def updi_write_fuse(self, fuse: int, value: int) -> bool:
        """Write UPDI fuse"""
        cmd_data = bytes([fuse, value])
        success, _ = self._execute(CMD_UPDI_WRITE_FUSE, cmd_data)
        return success

    def updi_read_eeprom(self, address: int, length: int) -> Optional[bytes]:
        """Read from UPDI target EEPROM"""
        cmd_data = address.to_bytes(2, "little") + length.to_bytes(2, "little")
        status, data = self._execute_with_status(0x76, cmd_data)
        if status == STAT_OK_WITH_DATA:
            return data
        return None

    def updi_write_eeprom(self, address: int, data: bytes) -> bool:
        """Write to UPDI target EEPROM"""
        cmd_data = address.to_bytes(2, "little") + data
        success, _ = self._execute(0x77, cmd_data)
        return success

    def onewire_init(self) -> bool:
        """Initialize 1-Wire interface"""
        success, _ = self._execute(CMD_1WIRE_INIT)
        return success

    def onewire_deinit(self) -> bool:
        """Deinitialize 1-Wire interface"""
        success, _ = self._execute(CMD_1WIRE_DEINIT)
        return success

    def onewire_reset(self) -> bool:
        """Reset 1-Wire bus"""
        success, _ = self._execute(CMD_1WIRE_RESET)
        return success

    def onewire_search(self) -> List[Dict[str, Any]]:
        """Search for 1-Wire devices"""
        status, data = self._execute_with_status(CMD_1WIRE_SEARCH)
        devices = []
        if status == STAT_OK_WITH_DATA and data:
            offset = 0
            while offset + 8 <= len(data):
                rom = list(data[offset : offset + 8])
                crc = sum(rom) & 0xFF
                devices.append(
                    {
                        "rom": rom,
                        "family": rom[0],
                        "crc_ok": crc == 0,
                    }
                )
                offset += 9
        return devices

    def onewire_read_rom(self) -> Optional[List[int]]:
        """Read ROM from single 1-Wire device"""
        status, data = self._execute_with_status(CMD_1WIRE_READ_ROM)
        if status == STAT_OK_WITH_DATA and len(data) >= 8:
            return list(data[:8])
        return None

    def onewire_read_temperature(self, rom: Optional[List[int]] = None) -> Optional[float]:
        """Read temperature from DS18B20"""
        cmd_data = bytes(rom) if rom else bytes(8)
        status, data = self._execute_with_status(CMD_1WIRE_READ_TEMP, cmd_data, timeout=2.0)
        if status == STAT_OK_WITH_DATA and len(data) >= 2:
            raw = int.from_bytes(data[:2], "little")
            if raw & 0x8000:
                raw = -(raw ^ 0xFFFF) - 1
            return raw / 16.0
        return None
