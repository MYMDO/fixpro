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
    sendResponse("CAPS:i2c,spi,gpio");
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
    Serial.println("  PING       - Connection test (returns CAFE)");
    Serial.println("  CAPS       - Get device capabilities");
    Serial.println("  VERSION    - Firmware version");
    Serial.println("  GPIO       - Read GPIO states");
    Serial.println("  GPIO_SET   - Set LED on");
    Serial.println("  GPIO_CLR   - Set LED off");
    Serial.println("  SPI_ID     - Read SPI flash JEDEC ID");
    Serial.println("  I2C_SCAN   - Scan I2C bus for devices");
    Serial.println("  STATUS     - System status");
    Serial.println("  INFO       - Device information");
    Serial.println("  HELP       - Show this help");
}

void process_command(const char* cmd) {
    if (strcmp(cmd, "PING") == 0) {
        cmd_ping();
    } else if (strcmp(cmd, "CAPS") == 0) {
        cmd_caps();
    } else if (strcmp(cmd, "VERSION") == 0) {
        cmd_version();
    } else if (strcmp(cmd, "GPIO") == 0 || strcmp(cmd, "GPIO_TEST") == 0) {
        cmd_gpio();
    } else if (strcmp(cmd, "GPIO_SET") == 0) {
        cmd_gpio_set();
    } else if (strcmp(cmd, "GPIO_CLR") == 0) {
        cmd_gpio_clr();
    } else if (strcmp(cmd, "SPI_ID") == 0) {
        cmd_spi_id();
    } else if (strcmp(cmd, "I2C_SCAN") == 0) {
        cmd_i2c_scan();
    } else if (strcmp(cmd, "STATUS") == 0) {
        cmd_status();
    } else if (strcmp(cmd, "INFO") == 0) {
        cmd_info();
    } else if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "?") == 0) {
        cmd_help();
    } else if (strlen(cmd) > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "UNKNOWN:%s", cmd);
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
