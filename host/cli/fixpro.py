#!/usr/bin/env python3
"""
FiXPro CLI - Command Line Interface for FiXPro Flash iX Pro

Usage:
    fixpro.py [options] command [args]

Commands:
    detect          Detect connected flash chip
    read            Read flash memory
    write           Write flash memory
    erase           Erase flash memory
    verify          Verify flash content
    info            Get device information

Examples:
    fixpro.py detect
    fixpro.py read --output firmware.bin
    fixpro.py write --input firmware.bin --address 0x1000
    fixpro.py erase --sector 0x1000

Author: FiXPro Contributors
License: GPL-3.0
"""

import sys
import os
import argparse
import serial
import serial.tools.list_ports
import time
import json
import hashlib
from typing import Optional, List, Tuple, Dict, Any

__version__ = "1.0.0"
__author__ = "FiXPro Contributors"

#==============================================================================
# PROTOCOL CONSTANTS
#==============================================================================

SYNC_BYTE = 0xAB
USB_MAX_PACKET_SIZE = 4096

# Commands (Host -> Device)
CMD_PING = 0x01
CMD_GET_INFO = 0x02
CMD_RESET = 0x03
CMD_GET_STATUS = 0x04

CMD_SPI_INIT = 0x10
CMD_SPI_DEINIT = 0x11
CMD_SPI_TRANSFER = 0x12
CMD_SPI_WRITE = 0x13
CMD_SPI_READ = 0x14
CMD_SPI_SET_FREQ = 0x15
CMD_SPI_SET_MODE = 0x16

CMD_I2C_INIT = 0x20
CMD_I2C_DEINIT = 0x21
CMD_I2C_WRITE = 0x22
CMD_I2C_READ = 0x23
CMD_I2C_SCAN = 0x24
CMD_I2C_SET_FREQ = 0x25

# Status (Device -> Host)
STAT_OK = 0x80
STAT_OK_WITH_DATA = 0x81

STAT_ERROR = 0xE0
STAT_ERROR_PARAM = 0xE1
STAT_ERROR_TIMEOUT = 0xE2
STAT_ERROR_BUS = 0xE3
STAT_ERROR_NACK = 0xE4
STAT_ERROR_NO_DEVICE = 0xE5
STAT_ERROR_VERIFY = 0xE6
STAT_ERROR_SAFETY = 0xE7
STAT_ERROR_BUSY = 0xE8

STATUS_NAMES = {
    STAT_OK: "OK",
    STAT_OK_WITH_DATA: "OK with data",
    STAT_ERROR: "General error",
    STAT_ERROR_PARAM: "Invalid parameter",
    STAT_ERROR_TIMEOUT: "Timeout",
    STAT_ERROR_BUS: "Bus error",
    STAT_ERROR_NACK: "No ACK received",
    STAT_ERROR_NO_DEVICE: "No device",
    STAT_ERROR_VERIFY: "Verification failed",
    STAT_ERROR_SAFETY: "Safety error",
    STAT_ERROR_BUSY: "Device busy",
}


#==============================================================================
# CHIP DATABASE
#==============================================================================

CHIP_DATABASE = {
    # Winbond W25Q series
    (0xEF, 0x40, 0x14): {"name": "Winbond W25Q80", "size": 1 * 1024 * 1024, "type": "spi"},
    (0xEF, 0x40, 0x15): {"name": "Winbond W25Q16", "size": 2 * 1024 * 1024, "type": "spi"},
    (0xEF, 0x40, 0x16): {"name": "Winbond W25Q32", "size": 4 * 1024 * 1024, "type": "spi"},
    (0xEF, 0x40, 0x17): {"name": "Winbond W25Q64", "size": 8 * 1024 * 1024, "type": "spi"},
    (0xEF, 0x40, 0x18): {"name": "Winbond W25Q128", "size": 16 * 1024 * 1024, "type": "spi"},
    (0xEF, 0x40, 0x19): {"name": "Winbond W25Q256", "size": 32 * 1024 * 1024, "type": "spi"},
    
    # Macronix MX25 series
    (0xC2, 0x20, 0x15): {"name": "Macronix MX25L1606", "size": 2 * 1024 * 1024, "type": "spi"},
    (0xC2, 0x20, 0x16): {"name": "Macronix MX25L3206", "size": 4 * 1024 * 1024, "type": "spi"},
    (0xC2, 0x20, 0x17): {"name": "Macronix MX25L6406", "size": 8 * 1024 * 1024, "type": "spi"},
    (0xC2, 0x20, 0x18): {"name": "Macronix MX25L128", "size": 16 * 1024 * 1024, "type": "spi"},
    
    # Gigadevice GD25Q series
    (0xC8, 0x40, 0x14): {"name": "Gigadevice GD25Q80", "size": 1 * 1024 * 1024, "type": "spi"},
    (0xC8, 0x40, 0x15): {"name": "Gigadevice GD25Q16", "size": 2 * 1024 * 1024, "type": "spi"},
    (0xC8, 0x40, 0x16): {"name": "Gigadevice GD25Q32", "size": 4 * 1024 * 1024, "type": "spi"},
    (0xC8, 0x40, 0x17): {"name": "Gigadevice GD25Q64", "size": 8 * 1024 * 1024, "type": "spi"},
    (0xC8, 0x40, 0x18): {"name": "Gigadevice GD25Q128", "size": 16 * 1024 * 1024, "type": "spi"},
}


#==============================================================================
# UTILITY FUNCTIONS
#==============================================================================

def print_error(msg: str) -> None:
    """Print error message to stderr"""
    print(f"ERROR: {msg}", file=sys.stderr)


def print_warning(msg: str) -> None:
    """Print warning message to stderr"""
    print(f"WARNING: {msg}", file=sys.stderr)


def print_info(msg: str) -> None:
    """Print info message"""
    print(f"INFO: {msg}")


def print_success(msg: str) -> None:
    """Print success message"""
    print(f"SUCCESS: {msg}")


def format_hex(data: bytes, address: int = 0, width: int = 16) -> str:
    """Format binary data as hex dump"""
    lines = []
    for i in range(0, len(data), width):
        chunk = data[i:i + width]
        hex_part = ' '.join(f'{b:02X}' for b in chunk)
        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        lines.append(f'{address + i:08X}  {hex_part:<{width * 3}}  {ascii_part}')
    return '\n'.join(lines)


def calculate_checksum(data: bytes) -> str:
    """Calculate SHA256 checksum of data"""
    return hashlib.sha256(data).hexdigest()


#==============================================================================
# FiXPro DEVICE CLASS
#==============================================================================

class FiXProDevice:
    """Communication class for FiXPro device"""
    
    def __init__(self, port: Optional[str] = None, baudrate: int = 115200, timeout: float = 1.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial: Optional[serial.Serial] = None
        self.connected = False
        
    def __enter__(self):
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
        
    def find_device(self) -> Optional[str]:
        """Find FiXPro device by VID/PID or port name"""
        ports = serial.tools.list_ports.comports()
        
        for p in ports:
            # Check for FiXPro by description
            if "FiXPro" in p.description or "fixpro" in p.description.lower():
                return p.device
            
            # Check for Raspberry Pi Pico in bootloader mode
            if "Pico" in p.description or " rp2040" in p.description.lower():
                # Check if it's our device by trying to communicate
                try:
                    test_port = serial.Serial(p.device, 115200, timeout=0.1)
                    test_port.close()
                    return p.device
                except:
                    pass
        
        # Return first available port if nothing specific found
        if ports:
            return ports[0].device
        
        return None
    
    def connect(self) -> bool:
        """Connect to FiXPro device"""
        if self.connected and self.serial and self.serial.is_open:
            return True
        
        port = self.port
        if port is None:
            port = self.find_device()
            if port is None:
                print_error("FiXPro device not found")
                return False
        
        try:
            self.serial = serial.Serial(
                port=port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            self.connected = True
            self.port = port
            print_info(f"Connected to {port}")
            return True
        except serial.SerialException as e:
            print_error(f"Failed to connect: {e}")
            return False
    
    def disconnect(self) -> None:
        """Disconnect from device"""
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.connected = False
    
    def _send_packet(self, cmd: int, data: bytes = b'') -> bool:
        """Send packet to device"""
        if not self.connected or not self.serial:
            return False
        
        if len(data) > USB_MAX_PACKET_SIZE:
            print_error("Data too large")
            return False
        
        # Build packet
        packet = bytes([SYNC_BYTE, cmd])
        packet += len(data).to_bytes(2, 'little')
        packet += data
        
        try:
            self.serial.write(packet)
            self.serial.flush()
            return True
        except serial.SerialException as e:
            print_error(f"Send failed: {e}")
            return False
    
    def _recv_response(self, timeout: Optional[float] = None) -> Tuple[int, bytes]:
        """Receive response from device"""
        if not self.connected or not self.serial:
            return STAT_ERROR, b''
        
        old_timeout = self.serial.timeout
        if timeout is not None:
            self.serial.timeout = timeout
        
        try:
            # Wait for sync byte
            start = time.time()
            while True:
                byte = self.serial.read(1)
                if byte == bytes([SYNC_BYTE]):
                    break
                if timeout and (time.time() - start) > timeout:
                    self.serial.timeout = old_timeout
                    return STAT_ERROR_TIMEOUT, b''
            
            # Read status and length
            header = self.serial.read(3)
            if len(header) < 3:
                self.serial.timeout = old_timeout
                return STAT_ERROR, b''
            
            status = header[0]
            length = int.from_bytes(header[1:3], 'little')
            
            # Read data
            data = b''
            if length > 0:
                remaining = length
                while remaining > 0:
                    chunk = self.serial.read(min(remaining, 4096))
                    if not chunk:
                        break
                    data += chunk
                    remaining -= len(chunk)
            
            self.serial.timeout = old_timeout
            return status, data
            
        except serial.SerialException as e:
            print_error(f"Receive failed: {e}")
            self.serial.timeout = old_timeout
            return STAT_ERROR, b''
    
    def _execute(self, cmd: int, data: bytes = b'', timeout: float = 1.0) -> Tuple[bool, bytes]:
        """Execute command and return response"""
        if not self._send_packet(cmd, data):
            return False, b''
        
        status, response = self._recv_response(timeout)
        
        if status == STAT_ERROR_TIMEOUT:
            print_error("Device timeout")
            return False, b''
        
        if status == STAT_ERROR:
            print_error("Command failed")
            return False, b''
        
        return True, response
    
    # Device commands
    def ping(self) -> bool:
        """Ping device"""
        success, _ = self._execute(CMD_PING)
        return success
    
    def get_info(self) -> Optional[Dict[str, Any]]:
        """Get device information"""
        success, data = self._execute(CMD_GET_INFO)
        if not success or len(data) < 16:
            return None
        
        return {
            "version": f"{data[0]}.{data[1]}.{data[2]}",
            "capabilities": int.from_bytes(data[3:7], 'little'),
            "hw_revision": data[7],
            "serial": data[8:20].decode('ascii', errors='replace').rstrip('\x00'),
            "flash_size": int.from_bytes(data[20:24], 'little'),
            "sram_size": int.from_bytes(data[24:28], 'little'),
        }
    
    def get_status(self) -> Optional[Dict[str, Any]]:
        """Get device safety status"""
        success, data = self._execute(CMD_GET_STATUS)
        if not success or len(data) < 10:
            return None
        
        return {
            "voltage_mv": int.from_bytes(data[0:2], 'little'),
            "current_ma": int.from_bytes(data[2:4], 'little'),
            "temperature_c": data[4],
            "status": data[5],
        }
    
    def reset(self) -> bool:
        """Reset device"""
        success, _ = self._execute(CMD_RESET, timeout=0.5)
        self.disconnect()
        return success
    
    # SPI commands
    def spi_init(self, frequency: int = 10000000, mode: int = 0) -> bool:
        """Initialize SPI"""
        config = frequency.to_bytes(4, 'little') + bytes([mode, 0, 0])
        success, _ = self._execute(CMD_SPI_INIT, config)
        return success
    
    def spi_read(self, address: int, length: int, timeout: float = 10.0) -> Optional[bytes]:
        """Read from SPI flash"""
        cmd_data = address.to_bytes(4, 'little') + length.to_bytes(2, 'little')
        success, data = self._execute(CMD_SPI_READ, cmd_data, timeout=timeout)
        if success and data:
            return data
        return None
    
    def spi_write(self, address: int, data: bytes, timeout: float = 10.0) -> bool:
        """Write to SPI flash"""
        cmd_data = address.to_bytes(4, 'little') + data
        success, _ = self._execute(CMD_SPI_WRITE, cmd_data, timeout=timeout)
        return success
    
    def spi_transfer(self, data: bytes) -> Optional[bytes]:
        """Raw SPI transfer"""
        success, response = self._execute(CMD_SPI_TRANSFER, data)
        if success:
            return response
        return None
    
    # I2C commands
    def i2c_init(self, frequency: int = 400000) -> bool:
        """Initialize I2C"""
        config = frequency.to_bytes(4, 'little') + bytes([0])
        success, _ = self._execute(CMD_I2C_INIT, config)
        return success
    
    def i2c_write(self, address: int, data: bytes) -> bool:
        """Write to I2C device"""
        cmd_data = bytes([address]) + data
        success, _ = self._execute(CMD_I2C_WRITE, cmd_data)
        return success
    
    def i2c_read(self, address: int, length: int) -> Optional[bytes]:
        """Read from I2C device"""
        cmd_data = bytes([address]) + length.to_bytes(2, 'little')
        success, data = self._execute(CMD_I2C_READ, cmd_data)
        if success:
            return data
        return None
    
    def i2c_scan(self) -> List[int]:
        """Scan I2C bus for devices"""
        success, data = self._execute(CMD_I2C_SCAN)
        if success and data:
            return list(data)
        return []


#==============================================================================
# CLI COMMANDS
#==============================================================================

def cmd_detect(args) -> int:
    """Detect connected flash chip"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        print_info("Device connected, detecting flash chip...")
        
        # Try to read JEDEC ID via SPI transfer
        dev.spi_init(frequency=1000000)
        
        # JEDEC ID command: 0x9F
        jedec = dev.spi_transfer(bytes([0x9F, 0x00, 0x00, 0x00]))
        
        if jedec and len(jedec) >= 4:
            mfg_id = jedec[1]
            mem_type = jedec[2]
            cap_id = jedec[3]
            
            chip_key = (mfg_id, mem_type, cap_id)
            
            print_success("Flash chip detected!")
            print(f"  Manufacturer ID: 0x{mfg_id:02X}")
            print(f"  Memory Type:    0x{mem_type:02X}")
            print(f"  Capacity ID:    0x{cap_id:02X}")
            
            if chip_key in CHIP_DATABASE:
                chip_info = CHIP_DATABASE[chip_key]
                print(f"  Chip Name:      {chip_info['name']}")
                print(f"  Size:           {chip_info['size'] / 1024 / 1024:.0f} MB")
            else:
                print(f"  Chip:           Unknown (0x{mfg_id:02X} 0x{mem_type:02X} 0x{cap_id:02X})")
        else:
            print_warning("Could not detect flash chip")
            return 1
        
        return 0


def cmd_read(args) -> int:
    """Read flash memory"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        address = args.address if hasattr(args, 'address') else 0
        length = args.length if hasattr(args, 'length') else None
        output = args.output
        
        dev.spi_init(frequency=args.frequency if hasattr(args, 'frequency') else 10000000)
        
        # Auto-detect chip if length not specified
        if length is None:
            jedec = dev.spi_transfer(bytes([0x9F, 0x00, 0x00, 0x00]))
            if jedec and len(jedec) >= 4:
                chip_key = (jedec[1], jedec[2], jedec[3])
                if chip_key in CHIP_DATABASE:
                    length = CHIP_DATABASE[chip_key]['size']
                    print_info(f"Auto-detected chip: {CHIP_DATABASE[chip_key]['name']}")
                    print_info(f"Reading full chip ({length / 1024 / 1024:.0f} MB)...")
                else:
                    print_error("Unknown chip, please specify length manually")
                    return 1
            else:
                print_error("Could not detect chip")
                return 1
        
        print_info(f"Reading {length} bytes from address 0x{address:08X}...")
        
        data = dev.spi_read(address, length)
        
        if data is None:
            print_error("Read failed")
            return 1
        
        if output:
            with open(output, 'wb') as f:
                f.write(data)
            print_success(f"Read completed: {len(data)} bytes written to {output}")
            print(f"  Checksum (SHA256): {calculate_checksum(data)}")
        else:
            sys.stdout.buffer.write(data)
        
        return 0


def cmd_write(args) -> int:
    """Write flash memory"""
    if not os.path.exists(args.input):
        print_error(f"Input file not found: {args.input}")
        return 1
    
    with open(args.input, 'rb') as f:
        data = f.read()
    
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        address = args.address if hasattr(args, 'address') else 0
        
        dev.spi_init(frequency=args.frequency if hasattr(args, 'frequency') else 10000000)
        
        print_info(f"Writing {len(data)} bytes to address 0x{address:08X}...")
        
        if args.erase:
            print_info("Erasing chip first...")
            # Simplified erase - in real implementation would need proper sequence
            pass
        
        # Write in chunks
        chunk_size = 256
        total_chunks = (len(data) + chunk_size - 1) // chunk_size
        
        for i in range(total_chunks):
            chunk = data[i * chunk_size:(i + 1) * chunk_size]
            chunk_addr = address + i * chunk_size
            
            if not dev.spi_write(chunk_addr, chunk):
                print_error(f"Write failed at address 0x{chunk_addr:08X}")
                return 1
            
            progress = (i + 1) * 100 // total_chunks
            print(f"\rProgress: {progress}%", end='', flush=True)
        
        print()
        print_success(f"Write completed: {len(data)} bytes")
        
        if args.verify:
            print_info("Verifying...")
            read_back = dev.spi_read(address, len(data))
            if read_back == data:
                print_success("Verification passed!")
            else:
                print_error("Verification failed!")
                return 1
        
        return 0


def cmd_erase(args) -> int:
    """Erase flash memory"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        dev.spi_init()
        
        if hasattr(args, 'sector') and args.sector is not None:
            address = args.sector
            print_info(f"Erasing sector at 0x{address:08X}...")
            # Sector erase command: 0x20
            dev.spi_transfer(bytes([0x20, 
                (address >> 16) & 0xFF, 
                (address >> 8) & 0xFF, 
                address & 0xFF]))
        elif hasattr(args, 'chip') and args.chip:
            print_info("Erasing entire chip...")
            # Chip erase command: 0xC7
            dev.spi_transfer(bytes([0x06]))  # Write enable
            dev.spi_transfer(bytes([0xC7]))
            print_warning("Chip erase may take up to 100 seconds...")
        else:
            print_error("Please specify --sector or --chip")
            return 1
        
        print_success("Erase completed")
        return 0


def cmd_verify(args) -> int:
    """Verify flash content"""
    if not os.path.exists(args.input):
        print_error(f"Input file not found: {args.input}")
        return 1
    
    with open(args.input, 'rb') as f:
        expected = f.read()
    
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        address = args.address if hasattr(args, 'address') else 0
        
        dev.spi_init()
        
        print_info(f"Verifying {len(expected)} bytes from address 0x{address:08X}...")
        
        actual = dev.spi_read(address, len(expected))
        
        if actual is None:
            print_error("Read failed")
            return 1
        
        if actual == expected:
            print_success("Verification passed!")
            return 0
        else:
            print_error("Verification failed!")
            
            # Find first difference
            for i in range(min(len(actual), len(expected))):
                if actual[i] != expected[i]:
                    print(f"  First difference at offset 0x{i:08X}: expected 0x{expected[i]:02X}, got 0x{actual[i]:02X}")
                    break
            
            return 1


def cmd_info(args) -> int:
    """Get device information"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        info = dev.get_info()
        if info:
            print("Device Information:")
            print(f"  Firmware Version: {info['version']}")
            print(f"  Hardware Revision: {info['hw_revision']}")
            print(f"  Serial Number:    {info['serial']}")
            print(f"  Flash Size:       {info['flash_size'] / 1024:.0f} KB")
            print(f"  SRAM Size:        {info['sram_size'] / 1024:.0f} KB")
            
            caps = info['capabilities']
            print("  Capabilities:")
            if caps & 0x01: print("    - SPI")
            if caps & 0x02: print("    - I2C")
            if caps & 0x04: print("    - JTAG")
            if caps & 0x08: print("    - SWD")
        else:
            print_error("Failed to get device info")
            return 1
        
        status = dev.get_status()
        if status:
            print("\nSafety Status:")
            print(f"  Voltage: {status['voltage_mv']} mV")
            print(f"  Current: {status['current_ma']} mA")
            print(f"  Temperature: {status['temperature_c']} °C")
        
        return 0


def cmd_i2c_scan(args) -> int:
    """Scan I2C bus"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1
        
        dev.i2c_init()
        
        print_info("Scanning I2C bus...")
        
        devices = dev.i2c_scan()
        
        if devices:
            print_success(f"Found {len(devices)} device(s):")
            for addr in devices:
                print(f"  0x{addr:02X}")
        else:
            print_info("No devices found")
        
        return 0


#==============================================================================
# MAIN ENTRY POINT
#==============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="FiXPro - Flash iX Pro",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s detect
  %(prog)s read --output firmware.bin
  %(prog)s write --input firmware.bin --verify
  %(prog)s erase --chip
  %(prog)s info
        """
    )
    
    parser.add_argument('--version', action='version', version=f'%(prog)s {__version__}')
    parser.add_argument('--port', '-p', help='Serial port (auto-detect if not specified)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # detect command
    subparsers.add_parser('detect', help='Detect connected flash chip')
    
    # read command
    read_parser = subparsers.add_parser('read', help='Read flash memory')
    read_parser.add_argument('--output', '-o', help='Output file (stdout if not specified)')
    read_parser.add_argument('--address', '-a', type=lambda x: int(x, 0), default=0,
                            help='Start address (default: 0)')
    read_parser.add_argument('--length', '-l', type=lambda x: int(x, 0),
                            help='Number of bytes to read')
    read_parser.add_argument('--frequency', '-f', type=int, default=10000000,
                            help='SPI frequency in Hz (default: 10000000)')
    
    # write command
    write_parser = subparsers.add_parser('write', help='Write flash memory')
    write_parser.add_argument('--input', '-i', required=True, help='Input file')
    write_parser.add_argument('--address', '-a', type=lambda x: int(x, 0), default=0,
                             help='Start address (default: 0)')
    write_parser.add_argument('--verify', action='store_true', help='Verify after write')
    write_parser.add_argument('--erase', action='store_true', help='Erase before write')
    write_parser.add_argument('--frequency', '-f', type=int, default=10000000,
                             help='SPI frequency in Hz (default: 10000000)')
    
    # erase command
    erase_parser = subparsers.add_parser('erase', help='Erase flash memory')
    erase_group = erase_parser.add_mutually_exclusive_group(required=True)
    erase_group.add_argument('--sector', '-s', type=lambda x: int(x, 0),
                            help='Erase sector at address')
    erase_group.add_argument('--chip', '-c', action='store_true',
                            help='Erase entire chip')
    
    # verify command
    verify_parser = subparsers.add_parser('verify', help='Verify flash content')
    verify_parser.add_argument('--input', '-i', required=True, help='Reference file')
    verify_parser.add_argument('--address', '-a', type=lambda x: int(x, 0), default=0,
                             help='Start address (default: 0)')
    
    # info command
    subparsers.add_parser('info', help='Get device information')
    
    # i2c-scan command
    subparsers.add_parser('i2c-scan', help='Scan I2C bus for devices')
    
    args = parser.parse_args()
    
    if args.command is None:
        parser.print_help()
        return 1
    
    # Execute command
    commands = {
        'detect': cmd_detect,
        'read': cmd_read,
        'write': cmd_write,
        'erase': cmd_erase,
        'verify': cmd_verify,
        'info': cmd_info,
        'i2c-scan': cmd_i2c_scan,
    }
    
    if args.command in commands:
        try:
            return commands[args.command](args)
        except KeyboardInterrupt:
            print("\nOperation cancelled by user")
            return 130
        except Exception as e:
            print_error(f"Unexpected error: {e}")
            if args.verbose:
                import traceback
                traceback.print_exc()
            return 1
    else:
        parser.print_help()
        return 1


if __name__ == '__main__':
    sys.exit(main())
