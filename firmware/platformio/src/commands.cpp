/**
 * @file commands.cpp
 * @brief FiXPro Command Implementation
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "commands.h"

static char cmdBuffer[CMD_BUFFER_SIZE];
static int cmdPos = 0;

static void sendResponse(const char* msg) {
    Serial.println(msg);
}

static void sendResponsef(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.println(buf);
}

void cmd_ping(void) {
    sendResponse("CAFE");
}

void cmd_caps(void) {
    sendResponse("CAPS:spi,i2c,gpio,spi_read,spi_write,spi_erase,i2c_read,i2c_write");
}

void cmd_version(void) {
    sendResponsef("FiXPro v%s", FIXPRO_VERSION);
}

void cmd_gpio(void) {
    uint32_t gpioState = 0;
    for (int i = 0; i < 8; i++) {
        int pin = 10 + i;
        if (digitalRead(pin) == HIGH) {
            gpioState |= (1 << i);
        }
    }
    sendResponsef("GPIO:%08lX", gpioState);
}

void cmd_gpio_set(void) {
    digitalWrite(FIXPRO_LED_PIN, HIGH);
    sendResponse("OK");
}

void cmd_gpio_clr(void) {
    digitalWrite(FIXPRO_LED_PIN, LOW);
    sendResponse("OK");
}

void cmd_spi_id(void) {
    digitalWrite(FIXPRO_SPI_CS, LOW);
    delayMicroseconds(1);
    
    uint8_t txBuf[4] = {0x9F, 0x00, 0x00, 0x00};
    uint8_t rxBuf[4] = {0};
    
    SPI.transfer(txBuf, rxBuf, 4);
    
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    
    sendResponsef("SPI:%02X%02X%02X", rxBuf[1], rxBuf[2], rxBuf[3]);
}

void cmd_spi_read(uint32_t addr, uint32_t len) {
    if (len > 256) len = 256;
    
    digitalWrite(FIXPRO_SPI_CS, LOW);
    
    uint8_t cmd[4] = {0x03, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
    SPI.transfer(cmd, NULL, 4);
    
    uint8_t rxBuf[256];
    for (uint32_t i = 0; i < len; i++) {
        rxBuf[i] = SPI.transfer(0xFF);
    }
    
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    
    char response[512];
    char* p = response;
    p += snprintf(p, sizeof(response), "SPI_READ:%08lX:", addr);
    for (uint32_t i = 0; i < len && (p - response) < (int)(sizeof(response) - 3); i++) {
        p += snprintf(p, 4, "%02X ", rxBuf[i]);
    }
    sendResponse(response);
}

static void spi_write_enable(void) {
    digitalWrite(FIXPRO_SPI_CS, LOW);
    SPI.transfer(0x06);
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    delayMicroseconds(1);
}

static uint8_t spi_read_status(void) {
    digitalWrite(FIXPRO_SPI_CS, LOW);
    SPI.transfer(0x05);
    uint8_t status = SPI.transfer(0xFF);
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    return status;
}

static void spi_wait_ready(void) {
    uint32_t timeout = 100000;
    while (--timeout) {
        uint8_t status = spi_read_status();
        if ((status & 0x01) == 0) return;
        delayMicroseconds(10);
    }
}

void cmd_spi_erase(uint32_t addr, uint32_t len) {
    if (len > 4096) len = 4096;
    
    sendResponsef("SPI_ERASE:%08lX %lu", addr, len);
    
    spi_write_enable();
    
    digitalWrite(FIXPRO_SPI_CS, LOW);
    SPI.transfer(0x20);
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    
    spi_wait_ready();
    
    sendResponse("SPI_ERASE:OK");
}

void cmd_spi_erase_chip(void) {
    sendResponse("SPI_ERASE_CHIP:START");
    
    spi_write_enable();
    
    digitalWrite(FIXPRO_SPI_CS, LOW);
    SPI.transfer(0xC7);
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    
    spi_wait_ready();
    
    sendResponse("SPI_ERASE_CHIP:OK");
}

void cmd_spi_write(uint32_t addr, uint32_t len) {
    sendResponsef("SPI_WRITE:Use SPI_WRITE <addr> <hexdata>");
}

void cmd_i2c_scan(void) {
    String result = "I2C:";
    for (int addr = 0x03; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%02X ", addr);
            result += buf;
        }
    }
    sendResponse(result.c_str());
}

void cmd_i2c_read(uint8_t addr, uint8_t reg, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(addr, len);
    
    String result = "I2C_READ:";
    while (Wire.available() && len-- > 0) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X ", Wire.read());
        result += buf;
    }
    sendResponse(result.c_str());
}

void cmd_i2c_write(uint8_t addr, uint8_t reg, uint8_t data) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data);
    uint8_t status = Wire.endTransmission();
    
    if (status == 0) {
        sendResponse("I2C_WRITE:OK");
    } else {
        sendResponsef("I2C_WRITE:ERR_%d", status);
    }
}

void cmd_status(void) {
    sendResponsef("STATUS:v%s,sys=133MHz", FIXPRO_VERSION);
}

void cmd_info(void) {
    Serial.println("FiXPro Universal Programmer");
    Serial.print("Version: ");
    Serial.println(FIXPRO_VERSION);
    Serial.println("Platform: RP2040 Arduino");
    Serial.println("Capabilities: SPI, I2C, GPIO");
}

void cmd_help(void) {
    Serial.println("FiXPro Commands:");
    Serial.println("  PING         - Connection test (returns CAFE)");
    Serial.println("  CAPS         - Get device capabilities");
    Serial.println("  VERSION      - Firmware version");
    Serial.println("  GPIO         - Read GPIO states");
    Serial.println("  GPIO_SET     - Set LED on");
    Serial.println("  GPIO_CLR     - Set LED off");
    Serial.println("  SPI_ID       - Read SPI flash JEDEC ID");
    Serial.println("  SPI_READ     - Read SPI flash <addr> <len>");
    Serial.println("  SPI_ERASE    - Erase SPI sector <addr> <len>");
    Serial.println("  SPI_ERASE_CHIP - Full chip erase");
    Serial.println("  I2C_SCAN     - Scan I2C bus for devices");
    Serial.println("  I2C_READ     - Read I2C <addr> <reg> <len>");
    Serial.println("  I2C_WRITE    - Write I2C <addr> <reg> <data>");
    Serial.println("  STATUS       - System status");
    Serial.println("  INFO         - Device information");
    Serial.println("  HELP         - Show this help");
}

void process_command(const char* cmd) {
    char cmdCopy[CMD_BUFFER_SIZE];
    strncpy(cmdCopy, cmd, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';
    
    char* args[8];
    int argc = 0;
    char* token = strtok(cmdCopy, " \t");
    while (token && argc < 8) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    
    if (argc == 0) return;
    
    if (strcmp(args[0], "PING") == 0) {
        cmd_ping();
    } else if (strcmp(args[0], "CAPS") == 0) {
        cmd_caps();
    } else if (strcmp(args[0], "VERSION") == 0) {
        cmd_version();
    } else if (strcmp(args[0], "GPIO") == 0 || strcmp(args[0], "GPIO_TEST") == 0) {
        cmd_gpio();
    } else if (strcmp(args[0], "GPIO_SET") == 0) {
        cmd_gpio_set();
    } else if (strcmp(args[0], "GPIO_CLR") == 0) {
        cmd_gpio_clr();
    } else if (strcmp(args[0], "SPI_ID") == 0) {
        cmd_spi_id();
    } else if (strcmp(args[0], "SPI_READ") == 0 && argc >= 3) {
        uint32_t addr = strtoul(args[1], NULL, 0);
        uint32_t len = strtoul(args[2], NULL, 0);
        cmd_spi_read(addr, len);
    } else if (strcmp(args[0], "SPI_ERASE") == 0 && argc >= 3) {
        uint32_t addr = strtoul(args[1], NULL, 0);
        uint32_t len = strtoul(args[2], NULL, 0);
        cmd_spi_erase(addr, len);
    } else if (strcmp(args[0], "SPI_ERASE_CHIP") == 0) {
        cmd_spi_erase_chip();
    } else if (strcmp(args[0], "I2C_SCAN") == 0) {
        cmd_i2c_scan();
    } else if (strcmp(args[0], "I2C_READ") == 0 && argc >= 4) {
        uint8_t addr = strtoul(args[1], NULL, 0);
        uint8_t reg = strtoul(args[2], NULL, 0);
        uint8_t len = strtoul(args[3], NULL, 0);
        cmd_i2c_read(addr, reg, len);
    } else if (strcmp(args[0], "I2C_WRITE") == 0 && argc >= 4) {
        uint8_t addr = strtoul(args[1], NULL, 0);
        uint8_t reg = strtoul(args[2], NULL, 0);
        uint8_t data = strtoul(args[3], NULL, 0);
        cmd_i2c_write(addr, reg, data);
    } else if (strcmp(args[0], "STATUS") == 0) {
        cmd_status();
    } else if (strcmp(args[0], "INFO") == 0) {
        cmd_info();
    } else if (strcmp(args[0], "HELP") == 0 || strcmp(args[0], "?") == 0) {
        cmd_help();
    } else if (strlen(args[0]) > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "UNKNOWN:%s", args[0]);
        sendResponse(buf);
    }
}

void process_char(char c) {
    if (c == '\r' || c == '\n') {
        if (cmdPos > 0) {
            cmdBuffer[cmdPos] = '\0';
            process_command(cmdBuffer);
            cmdPos = 0;
        }
    } else if (cmdPos < CMD_BUFFER_SIZE - 1) {
        if (c >= 32 && c < 127) {
            cmdBuffer[cmdPos++] = c;
        }
    }
}

void init_commands(void) {
    cmdPos = 0;
    memset(cmdBuffer, 0, CMD_BUFFER_SIZE);
}
