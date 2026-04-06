#!/usr/bin/env python3
"""
FiXPro CLI - Command Line Interface for FiXPro Flash iX Pro

Usage:
    fixpro [options] command [args]

Commands:
    detect          Detect connected flash chip
    read            Read flash memory
    write           Write flash memory
    erase           Erase flash memory
    verify          Verify flash content
    info            Get device information

Author: FiXPro Contributors
License: GPL-3.0
"""

import argparse
import hashlib
import os
import sys
from typing import Any, Dict, List, Optional

# Add parent to path for standalone execution
if __name__ == "__main__":
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from cli.device import FiXProDevice
    from cli.protocols import (
        CAP_1WIRE,
        CAP_FLASH_READ,
        CAP_FLASH_WRITE,
        CAP_I2C,
        CAP_JTAG,
        CAP_SPI,
        CAP_SWD,
        CAP_UPDI,
    )
except ImportError:
    try:
        from fixpro.cli.device import FiXProDevice
        from fixpro.cli.protocols import (
            CAP_1WIRE,
            CAP_FLASH_READ,
            CAP_FLASH_WRITE,
            CAP_I2C,
            CAP_JTAG,
            CAP_SPI,
            CAP_SWD,
            CAP_UPDI,
        )
    except ImportError:
        from fixpro.cli.device import FiXProDevice
        from fixpro.cli.protocols import (
            CAP_1WIRE,
            CAP_FLASH_READ,
            CAP_FLASH_WRITE,
            CAP_I2C,
            CAP_JTAG,
            CAP_SPI,
            CAP_SWD,
            CAP_UPDI,
        )

__version__ = "2.0.0"
__author__ = "FiXPro Contributors"

SPI_FLASH_DB = {}
I2C_EEPROM_DB = {}
MCU_DB = {}
AVR_DB = {}


def print_error(msg: str) -> None:
    print(f"ERROR: {msg}", file=sys.stderr)


def print_warning(msg: str) -> None:
    print(f"WARNING: {msg}", file=sys.stderr)


def print_info(msg: str) -> None:
    print(f"INFO: {msg}")


def print_success(msg: str) -> None:
    print(f"SUCCESS: {msg}")


def load_chip_databases() -> None:
    """Load chip databases from JSON files"""
    import json

    script_dir = os.path.dirname(os.path.abspath(__file__))
    base_dir = os.path.dirname(os.path.dirname(script_dir))

    spi_db_path = os.path.join(base_dir, "chipdb", "spi_flash.json")
    eeprom_db_path = os.path.join(base_dir, "chipdb", "i2c_eeprom.json")
    mcu_db_path = os.path.join(base_dir, "chipdb", "mcu.json")
    avr_db_path = os.path.join(base_dir, "chipdb", "avr.json")

    if os.path.exists(spi_db_path):
        try:
            with open(spi_db_path, "r") as f:
                data = json.load(f)
                for chip in data.get("chips", []):
                    if "jedec" in chip:
                        key = tuple(int(x, 16) if isinstance(x, str) else x for x in chip["jedec"])
                        SPI_FLASH_DB[key] = chip
        except Exception as e:
            print_warning(f"Failed to load SPI flash database: {e}")

    if os.path.exists(eeprom_db_path):
        try:
            with open(eeprom_db_path, "r") as f:
                data = json.load(f)
                for chip in data.get("chips", []):
                    if "address" in chip:
                        for addr in chip["address"]:
                            I2C_EEPROM_DB[int(addr, 16) if isinstance(addr, str) else addr] = chip
        except Exception as e:
            print_warning(f"Failed to load I2C EEPROM database: {e}")

    if os.path.exists(mcu_db_path):
        try:
            with open(mcu_db_path, "r") as f:
                data = json.load(f)
                for mcu in data.get("chips", []):
                    if "idcode" in mcu:
                        MCU_DB[mcu["idcode"].upper()] = mcu
        except Exception as e:
            print_warning(f"Failed to load MCU database: {e}")

    if os.path.exists(avr_db_path):
        try:
            with open(avr_db_path, "r") as f:
                data = json.load(f)
                for chip in data.get("chips", []):
                    if "signature" in chip:
                        sig = tuple(
                            int(x, 16) if isinstance(x, str) else x for x in chip["signature"]
                        )
                        AVR_DB[sig] = chip
        except Exception as e:
            print_warning(f"Failed to load AVR database: {e}")

    if SPI_FLASH_DB:
        print_info(f"Loaded {len(SPI_FLASH_DB)} SPI flash chips")
    if I2C_EEPROM_DB:
        print_info(f"Loaded {len(I2C_EEPROM_DB)} I2C EEPROM chips")
    if MCU_DB:
        print_info(f"Loaded {len(MCU_DB)} MCU targets")


def lookup_spi_flash(mfg_id: int, mem_type: int, cap_id: int) -> Optional[Dict[str, Any]]:
    """Look up SPI flash chip by JEDEC ID"""
    key = (mfg_id, mem_type, cap_id)
    if key in SPI_FLASH_DB:
        return SPI_FLASH_DB[key]
    return None


def lookup_mcu(idcode: int) -> Optional[Dict[str, Any]]:
    """Look up MCU by IDCODE"""
    key = f"{idcode:08X}"
    return MCU_DB.get(key)


def lookup_avr(signature: List[int]) -> Optional[Dict[str, Any]]:
    """Look up AVR chip by signature"""
    key = tuple(signature[:3])
    if key in AVR_DB:
        return AVR_DB[key]
    return None


def cmd_detect(args) -> int:
    """Detect connected flash chip"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        print_info("Device connected, detecting flash chip...")

        dev.spi_init(frequency=1000000)
        jedec = dev.spi_transfer(bytes([0x9F, 0x00, 0x00, 0x00]))

        if jedec and len(jedec) >= 4:
            mfg_id, mem_type, cap_id = jedec[1], jedec[2], jedec[3]
            print_success("Flash chip detected!")
            print(f"  Manufacturer ID: 0x{mfg_id:02X}")
            print(f"  Memory Type:    0x{mem_type:02X}")
            print(f"  Capacity ID:    0x{cap_id:02X}")

            chip_info = lookup_spi_flash(mfg_id, mem_type, cap_id)
            if chip_info:
                print(f"  Chip Name:      {chip_info['name']}")
                size_mb = chip_info.get("size_mb", chip_info.get("size", 0) / 1024 / 1024)
                print(f"  Size:          {size_mb:.1f} MB")
                if "manufacturer" in chip_info:
                    print(f"  Manufacturer:  {chip_info['manufacturer']}")
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

        address = args.address if hasattr(args, "address") else 0
        length = args.length if hasattr(args, "length") else None
        output = args.output

        dev.spi_init(frequency=args.frequency if hasattr(args, "frequency") else 10000000)

        if length is None:
            jedec = dev.spi_transfer(bytes([0x9F, 0x00, 0x00, 0x00]))
            if jedec and len(jedec) >= 4:
                chip_info = lookup_spi_flash(jedec[1], jedec[2], jedec[3])
                if chip_info:
                    length = chip_info.get("size", 4 * 1024 * 1024)
                    print_info(f"Auto-detected: {chip_info['name']}")

        if length is None:
            print_error("Unknown chip, please specify --length")
            return 1

        print_info(f"Reading {length} bytes from address 0x{address:08X}...")
        data = dev.spi_read(address, length)

        if data is None:
            print_error("Read failed")
            return 1

        if output:
            with open(output, "wb") as f:
                f.write(data)
            print_success(f"Read completed: {len(data)} bytes written to {output}")
            print(f"  Checksum (SHA256): {hashlib.sha256(data).hexdigest()}")
        else:
            sys.stdout.buffer.write(data)
        return 0


def cmd_write(args) -> int:
    """Write flash memory"""
    if not os.path.exists(args.input):
        print_error(f"Input file not found: {args.input}")
        return 1

    with open(args.input, "rb") as f:
        data = f.read()

    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        address = args.address if hasattr(args, "address") else 0
        dev.spi_init(frequency=args.frequency if hasattr(args, "frequency") else 10000000)

        print_info(f"Writing {len(data)} bytes to address 0x{address:08X}...")

        chunk_size = 256
        total_chunks = (len(data) + chunk_size - 1) // chunk_size

        for i in range(total_chunks):
            chunk = data[i * chunk_size : (i + 1) * chunk_size]
            chunk_addr = address + i * chunk_size
            if not dev.spi_write(chunk_addr, chunk):
                print_error(f"Write failed at address 0x{chunk_addr:08X}")
                return 1
            progress = (i + 1) * 100 // total_chunks
            print(f"\rProgress: {progress}%", end="", flush=True)

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

        if hasattr(args, "sector") and args.sector is not None:
            address = args.sector
            print_info(f"Erasing sector at 0x{address:08X}...")
            dev.spi_transfer(
                bytes([0x20, (address >> 16) & 0xFF, (address >> 8) & 0xFF, address & 0xFF])
            )
        elif hasattr(args, "chip") and args.chip:
            print_info("Erasing entire chip...")
            dev.spi_transfer(bytes([0x06]))
            dev.spi_transfer(bytes([0xC7]))
            print_warning("Chip erase may take up to 100 seconds...")

        print_success("Erase completed")
        return 0


def cmd_verify(args) -> int:
    """Verify flash content"""
    if not os.path.exists(args.input):
        print_error(f"Input file not found: {args.input}")
        return 1

    with open(args.input, "rb") as f:
        expected = f.read()

    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        address = args.address if hasattr(args, "address") else 0
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
            for i in range(min(len(actual), len(expected))):
                if actual[i] != expected[i]:
                    print(f"  First difference at offset 0x{i:08X}")
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

            caps = info["capabilities"]
            print("  Capabilities:")
            if caps & CAP_SPI:
                print("    - SPI")
            if caps & CAP_I2C:
                print("    - I2C")
            if caps & CAP_JTAG:
                print("    - JTAG")
            if caps & CAP_SWD:
                print("    - SWD")
            if caps & CAP_UPDI:
                print("    - UPDI")
            if caps & CAP_1WIRE:
                print("    - 1-Wire")
            if caps & CAP_FLASH_READ:
                print("    - Flash Read")
            if caps & CAP_FLASH_WRITE:
                print("    - Flash Write")

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


def cmd_jtag_idcode(args) -> int:
    """Read JTAG IDCODE"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.jtag_init():
            print_error("Failed to initialize JTAG")
            return 1

        dev.jtag_reset()
        idcode = dev.jtag_read_idcode()

        if idcode is not None:
            print_success("IDCODE read successfully")
            print(f"  IDCODE: 0x{idcode:08X}")

            if idcode != 0:
                mfg_id = (idcode >> 1) & 0x7FF
                part_num = (idcode >> 12) & 0xFFFF
                revision = (idcode >> 28) & 0xF
                print(f"  Manufacturer ID: 0x{mfg_id:03X}")
                print(f"  Part Number: 0x{part_num:04X}")
                print(f"  Revision: {revision}")

                mcu_info = lookup_mcu(idcode)
                if mcu_info:
                    print(f"  Device: {mcu_info['name']}")
                    if "core" in mcu_info:
                        print(f"  Core: {mcu_info['core']}")
            return 0
        else:
            print_error("Failed to read IDCODE")
            return 1


def cmd_swd_dap(args) -> int:
    """Connect to ARM DAP"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.swd_init():
            print_error("Failed to initialize SWD")
            return 1

        dev.swd_reset()
        print_info("Connecting to ARM Debug Access Port...")

        print("\nDP Registers:")
        dp_idr = dev.swd_read(0x0)
        if dp_idr:
            print(f"  DP-IDR:  0x{dp_idr:08X}")
        ctrl = dev.swd_read(0x4)
        if ctrl:
            print(f"  CTRL:   0x{ctrl:08X}")

        print_success("DAP scan complete")
        return 0


def cmd_updi_info(args) -> int:
    """Read UPDI device info"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.updi_init():
            print_error("Failed to initialize UPDI")
            return 1

        dev.updi_reset()

        info = dev.updi_read_info()
        if info:
            print_success("UPDI device detected!")
            sig = info["signature"]
            print(f"  Signature:   {' '.join(f'{b:02X}' for b in sig)}")
            print(f"  Revision:    {info['revision']}")
            print(f"  Flash Size:  {info['flash_size']} bytes")
            print(f"  EEPROM Size: {info['eeprom_size']} bytes")

            chip = lookup_avr(sig)
            if chip:
                print(f"  Device:     {chip['name']}")
                print(f"  Series:     {chip.get('series', 'N/A')}")
                print(f"  Package:    {chip.get('package', 'N/A')}")
                if "notes" in chip:
                    print(f"  Notes:      {chip['notes']}")
            return 0
        else:
            print_error("Failed to read device info")
            return 1


def cmd_updi_erase(args) -> int:
    """Erase UPDI target chip"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.updi_init():
            print_error("Failed to initialize UPDI")
            return 1

        print_warning("Erasing chip...")
        if dev.updi_erase_chip():
            print_success("Chip erased")
            return 0
        else:
            print_error("Erase failed")
            return 1


def cmd_updi_read_fuse(args) -> int:
    """Read UPDI fuse"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.updi_init():
            print_error("Failed to initialize UPDI")
            return 1

        fuse = args.fuse if hasattr(args, "fuse") else 0
        value = dev.updi_read_fuse(fuse)
        if value is not None:
            print_success(f"Fuse {fuse} = 0x{value:02X}")
            return 0
        else:
            print_error("Failed to read fuse")
            return 1


def cmd_updi_write_fuse(args) -> int:
    """Write UPDI fuse"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.updi_init():
            print_error("Failed to initialize UPDI")
            return 1

        fuse = args.fuse if hasattr(args, "fuse") else 0
        value = args.value if hasattr(args, "value") else 0
        if dev.updi_write_fuse(fuse, value):
            print_success(f"Fuse {fuse} = 0x{value:02X}")
            return 0
        else:
            print_error("Failed to write fuse")
            return 1


def cmd_onewire_scan(args) -> int:
    """Scan 1-Wire bus for devices"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.onewire_init():
            print_error("Failed to initialize 1-Wire")
            return 1

        print_info("Scanning 1-Wire bus...")

        devices = dev.onewire_search()
        if devices:
            print_success(f"Found {len(devices)} device(s):")
            for dev_info in devices:
                rom = dev_info["rom"]
                family = dev_info["family"]
                family_names = {
                    0x28: "DS18B20",
                    0x22: "DS1822",
                    0x3B: "DS1825",
                    0x26: "DS2433",
                    0x2D: "DS2431",
                }
                name = family_names.get(family, f"Unknown (0x{family:02X})")
                rom_str = ":".join(f"{b:02X}" for b in rom[:7])
                print(f"  {name} ({rom_str})")
        else:
            print_info("No devices found")
        return 0


def cmd_onewire_temp(args) -> int:
    """Read temperature from DS18B20"""
    with FiXProDevice() as dev:
        if not dev.ping():
            print_error("Device not responding")
            return 1

        if not dev.onewire_init():
            print_error("Failed to initialize 1-Wire")
            return 1

        print_info("Reading temperature...")

        temp = dev.onewire_read_temperature()
        if temp is not None:
            print_success(f"Temperature: {temp:.1f}°C")
            return 0
        else:
            print_error("Failed to read temperature")
            return 1


def cmd_list_chips(args) -> int:
    """List supported chips"""
    chip_type = args.type

    if chip_type in ("spi", "all"):
        print("\n=== SPI Flash Chips ===")
        for key, chip in sorted(SPI_FLASH_DB.items(), key=lambda x: x[1].get("name", "")):
            size = chip.get("size_mb", chip.get("size", 0) / 1024 / 1024)
            mfg = chip.get("manufacturer", "Unknown")
            jedec = chip.get("jedec", key)
            jedec_str = " ".join(str(b).upper() for b in jedec)
            print(
                f"  {chip.get('name', 'Unknown'):<25} {size:>6.1f} MB  {mfg:<15} JEDEC: {jedec_str}"
            )

    if chip_type in ("eeprom", "all"):
        print("\n=== I2C EEPROM Chips ===")
        shown = set()
        for chip in I2C_EEPROM_DB.values():
            name = chip.get("name", "Unknown")
            if name not in shown:
                shown.add(name)
                size = chip.get("size", 0)
                size_str = f"{size} B" if size < 1024 else f"{size / 1024:.0f} KB"
                mfg = chip.get("manufacturer", "Unknown")
                print(f"  {name:<20} {size_str:>10}  {mfg}")

    if chip_type in ("mcu", "all"):
        print("\n=== MCU Targets (JTAG/SWD) ===")
        for idcode, mcu in sorted(MCU_DB.items(), key=lambda x: x[1].get("name", "")):
            name = mcu.get("name", "Unknown")
            core = mcu.get("core", "Unknown")
            mfg = mcu.get("manufacturer", "Unknown")
            flash = mcu.get("flash_size", 0)
            flash_str = f"{flash / 1024:.0f} KB" if flash > 0 else "N/A"
            print(f"  {name:<30} {core:<25} {flash_str:>10}  {mfg}")

    if chip_type in ("avr", "updi", "all"):
        print("\n=== AVR UPDI Chips ===")
        for sig, chip in sorted(AVR_DB.items(), key=lambda x: x[1].get("name", "")):
            name = chip.get("name", "Unknown")
            flash = chip.get("flash_size", 0)
            eeprom = chip.get("eeprom_size", 0)
            series = chip.get("series", "AVR")
            flash_str = f"{flash / 1024:.0f} KB" if flash > 0 else "N/A"
            eeprom_str = f"{eeprom} B" if eeprom > 0 else "N/A"
            sig_str = " ".join(f"{b:02X}" for b in sig[:3])
            print(f"  {name:<20} {flash_str:>10}  {eeprom_str:>8}  {series:<10}  SIG: {sig_str}")

    total = len(SPI_FLASH_DB) + len(I2C_EEPROM_DB) + len(MCU_DB) + len(AVR_DB)
    print(f"\nTotal chips in database: {total}")
    return 0


def main():
    load_chip_databases()

    parser = argparse.ArgumentParser(
        description="FiXPro - Universal Hardware Programmer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  fixpro detect
  fixpro read -o firmware.bin
  fixpro write -i firmware.bin --verify
  fixpro erase --chip
  fixpro info
  fixpro i2c-scan
  fixpro jtag-idcode
  fixpro swd-dap
  fixpro list-chips --type mcu
        """,
    )

    parser.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    parser.add_argument("--port", "-p", help="Serial port (auto-detect if not specified)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    subparsers.add_parser("detect", help="Detect connected flash chip")

    read_parser = subparsers.add_parser("read", help="Read flash memory")
    read_parser.add_argument("--output", "-o", help="Output file")
    read_parser.add_argument("--address", "-a", type=lambda x: int(x, 0), default=0)
    read_parser.add_argument("--length", "-l", type=lambda x: int(x, 0))
    read_parser.add_argument("--frequency", "-f", type=int, default=10000000)

    write_parser = subparsers.add_parser("write", help="Write flash memory")
    write_parser.add_argument("--input", "-i", required=True)
    write_parser.add_argument("--address", "-a", type=lambda x: int(x, 0), default=0)
    write_parser.add_argument("--verify", action="store_true")
    write_parser.add_argument("--erase", action="store_true")
    write_parser.add_argument("--frequency", "-f", type=int, default=10000000)

    erase_parser = subparsers.add_parser("erase", help="Erase flash memory")
    erase_group = erase_parser.add_mutually_exclusive_group(required=True)
    erase_group.add_argument("--sector", "-s", type=lambda x: int(x, 0))
    erase_group.add_argument("--chip", "-c", action="store_true")

    verify_parser = subparsers.add_parser("verify", help="Verify flash content")
    verify_parser.add_argument("--input", "-i", required=True)
    verify_parser.add_argument("--address", "-a", type=lambda x: int(x, 0), default=0)

    subparsers.add_parser("info", help="Get device information")
    subparsers.add_parser("i2c-scan", help="Scan I2C bus")
    subparsers.add_parser("jtag-idcode", help="Read JTAG IDCODE")
    subparsers.add_parser("swd-dap", help="Connect to ARM DAP")

    subparsers.add_parser("updi-info", help="Read UPDI device info")
    subparsers.add_parser("updi-erase", help="Erase UPDI target chip")

    fuse_read_parser = subparsers.add_parser("updi-read-fuse", help="Read UPDI fuse")
    fuse_read_parser.add_argument("--fuse", "-n", type=int, default=0)

    fuse_write_parser = subparsers.add_parser("updi-write-fuse", help="Write UPDI fuse")
    fuse_write_parser.add_argument("--fuse", "-n", type=int, default=0)
    fuse_write_parser.add_argument("--value", "-v", type=lambda x: int(x, 0), required=True)

    subparsers.add_parser("1wire-scan", help="Scan 1-Wire bus")
    subparsers.add_parser("1wire-temp", help="Read DS18B20 temperature")

    list_parser = subparsers.add_parser("list-chips", help="List supported chips")
    list_parser.add_argument(
        "--type", choices=["spi", "eeprom", "mcu", "avr", "all"], default="all"
    )

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        return 1

    commands = {
        "detect": cmd_detect,
        "read": cmd_read,
        "write": cmd_write,
        "erase": cmd_erase,
        "verify": cmd_verify,
        "info": cmd_info,
        "i2c-scan": cmd_i2c_scan,
        "jtag-idcode": cmd_jtag_idcode,
        "swd-dap": cmd_swd_dap,
        "updi-info": cmd_updi_info,
        "updi-erase": cmd_updi_erase,
        "updi-read-fuse": cmd_updi_read_fuse,
        "updi-write-fuse": cmd_updi_write_fuse,
        "1wire-scan": cmd_onewire_scan,
        "1wire-temp": cmd_onewire_temp,
        "list-chips": cmd_list_chips,
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


if __name__ == "__main__":
    sys.exit(main())
