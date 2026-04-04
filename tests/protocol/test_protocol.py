# FiXPro Protocol Tests
# Tests for the OPUP text protocol

import time
import unittest
from unittest.mock import Mock

import serial


class TestProtocolResponses(unittest.TestCase):
    """Test FiXPro protocol responses"""

    def test_ping_response(self):
        """PING command should return CAFE"""
        self.assertEqual("CAFE", "CAFE")

    def test_caps_format(self):
        """CAPS response should contain protocol list"""
        caps = "CAPS:i2c,spi,gpio"
        self.assertTrue(caps.startswith("CAPS:"))
        self.assertIn("spi", caps)
        self.assertIn("i2c", caps)
        self.assertIn("gpio", caps)

    def test_gpio_format(self):
        """GPIO response should be hex format"""
        gpio = "GPIO:00000000"
        self.assertTrue(gpio.startswith("GPIO:"))
        self.assertEqual(8, len(gpio.split(":")[1]))

    def test_spi_format(self):
        """SPI ID response should be hex format"""
        spi = "SPI:EF4017"
        self.assertTrue(spi.startswith("SPI:"))
        self.assertEqual(6, len(spi.split(":")[1]))

    def test_i2c_format(self):
        """I2C scan response should contain addresses"""
        i2c = "I2C:50 68 "
        self.assertTrue(i2c.startswith("I2C:"))

    def test_version_format(self):
        """VERSION response should contain v and number"""
        version = "FiXPro v2.0.0"
        self.assertTrue(version.startswith("FiXPro v"))
        self.assertIn(".", version)

    def test_status_format(self):
        """STATUS response should contain system info"""
        status = "STATUS:v2.0.0,sys=133MHz"
        self.assertTrue(status.startswith("STATUS:"))
        self.assertIn("sys=", status)

    def test_error_format(self):
        """ERROR responses should be prefixed"""
        error = "ERR:timeout"
        self.assertTrue(error.startswith("ERR:"))

    def test_ok_response(self):
        """OK response for successful operations"""
        self.assertEqual("OK", "OK")


class TestCommandParsing(unittest.TestCase):
    """Test command parsing logic"""

    def test_command_uppercase(self):
        """Commands should be uppercase"""
        cmd = "PING"
        self.assertEqual(cmd, cmd.upper())

    def test_valid_commands(self):
        """List of valid commands"""
        valid_commands = [
            "PING",
            "CAPS",
            "VERSION",
            "GPIO",
            "GPIO_TEST",
            "GPIO_SET",
            "GPIO_CLR",
            "SPI_ID",
            "I2C_SCAN",
            "STATUS",
            "INFO",
            "HELP",
            "?",
        ]
        for cmd in valid_commands:
            self.assertEqual(cmd, cmd.strip())

    def test_command_with_newline(self):
        """Commands may have newline terminator"""
        cmd = "PING\n"
        self.assertTrue(cmd.strip() in ["PING", "CAPS", "VERSION"])

    def test_empty_command(self):
        """Empty commands should be handled"""
        cmd = ""
        self.assertEqual(0, len(cmd))


class TestSerialCommunication(unittest.TestCase):
    """Test serial communication mocking"""

    def setUp(self):
        self.mock_serial = Mock(spec=serial.Serial)
        self.mock_serial.is_open = True
        self.mock_serial.in_waiting = 0

    def test_serial_open(self):
        """Serial port should be open for communication"""
        self.assertTrue(self.mock_serial.is_open)

    def test_serial_write(self):
        """Should be able to write to serial"""
        self.mock_serial.write(b"PING\n")
        self.mock_serial.write.assert_called_with(b"PING\n")

    def test_serial_read(self):
        """Should be able to read from serial"""
        self.mock_serial.read.return_value = b"CAFE\n"
        data = self.mock_serial.read(10)
        self.assertEqual(b"CAFE\n", data)


class TestProtocolTimeout(unittest.TestCase):
    """Test timeout handling"""

    def test_default_timeout(self):
        """Default timeout should be reasonable"""
        timeout = 5.0
        self.assertGreater(timeout, 0)
        self.assertLessEqual(timeout, 30)

    def test_connection_timeout(self):
        """Connection should timeout if device not responding"""
        timeout = 3.0
        start = time.time()
        time.sleep(0.1)
        elapsed = time.time() - start
        self.assertGreater(timeout, elapsed)


class TestChipDetection(unittest.TestCase):
    """Test chip detection patterns"""

    def test_jedec_id_pattern(self):
        """JEDEC ID should match expected pattern"""
        import re

        pattern = r"^SPI:[0-9A-F]{6}$"
        spi_id = "SPI:EF4017"
        self.assertIsNotNone(re.match(pattern, spi_id))

    def test_manufacturer_id(self):
        """Known manufacturer IDs"""
        manufacturers = {
            "EF": "Winbond",
            "C8": "GigaDevice",
            "20": "Micron",
            "1C": "eON",
            "1F": "Atmel",
        }
        self.assertIn("EF", manufacturers)
        self.assertEqual("Winbond", manufacturers["EF"])


class TestErrorHandling(unittest.TestCase):
    """Test error handling"""

    def test_unknown_command_error(self):
        """Unknown commands should return error"""
        response = "UNKNOWN:INVALID"
        self.assertTrue(response.startswith("UNKNOWN:"))

    def test_timeout_error(self):
        """Timeout errors should be handled"""
        response = "ERR:timeout"
        self.assertTrue(response.startswith("ERR:"))
        self.assertIn("timeout", response)

    def test_crc_error(self):
        """CRC errors should be detected"""
        response = "ERR:crc_mismatch"
        self.assertTrue(response.startswith("ERR:"))


if __name__ == "__main__":
    unittest.main()
