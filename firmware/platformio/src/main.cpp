/**
 * FiXPro Universal Programmer - Text OPUP Protocol
 * RP2040 + Arduino Framework (Earle Philhower)
 * Web Serial API compatible
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#define LED_PIN 25
#define FLASH_CS 17
#define FLASH_MISO 16
#define FLASH_MOSI 19
#define FLASH_SCK 18

#define BUFFER_SIZE 128
char cmdBuffer[BUFFER_SIZE];
int cmdPos = 0;

unsigned long ledBlinkTime = 0;
bool ledState = false;

void ledBlink(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

void sendResponse(const String& msg) {
    Serial.println(msg);
}

void handlePing() {
    sendResponse("CAFE");
}

void handleCaps() {
    sendResponse("CAPS:i2c,spi,gpio,isp,swd");
}

void handleVersion() {
    sendResponse("FiXPro v2.0.0");
}

void handleGpio() {
    uint32_t gpioState = 0;
    for (int i = 0; i < 6; i++) {
        int pin = 10 + i;
        if (digitalRead(pin) == HIGH) {
            gpioState |= (1 << i);
        }
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "GPIO:%06lX", gpioState);
    sendResponse(buf);
}

void handleSpiId() {
    digitalWrite(FLASH_CS, LOW);
    delayMicroseconds(1);
    
    uint8_t txBuf[4] = {0x9F, 0x00, 0x00, 0x00};
    uint8_t rxBuf[4] = {0};
    
    SPI.transfer(txBuf, rxBuf, 4);
    
    digitalWrite(FLASH_CS, HIGH);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "SPI:%02X%02X%02X", rxBuf[1], rxBuf[2], rxBuf[3]);
    sendResponse(buf);
}

void handleI2cScan() {
    String result = "I2C:";
    for (int addr = 0x03; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X ", addr);
            result += buf;
        }
    }
    sendResponse(result);
}

void handleStatus() {
    char buf[64];
    snprintf(buf, sizeof(buf), "STATUS:sys=133MHz,ram=264KB,flash=0");
    sendResponse(buf);
}

void handleHelp() {
    Serial.println("FiXPro Commands:");
    Serial.println("  PING       - Connection test");
    Serial.println("  CAPS       - Get capabilities");
    Serial.println("  VERSION    - Firmware version");
    Serial.println("  GPIO       - Read GPIO states");
    Serial.println("  SPI_ID     - Read SPI flash ID");
    Serial.println("  I2C_SCAN   - Scan I2C devices");
    Serial.println("  STATUS     - System status");
    Serial.println("  HELP       - Show this help");
}

void processCommand(const char* cmd) {
    if (strcmp(cmd, "PING") == 0) {
        handlePing();
    } else if (strcmp(cmd, "CAPS") == 0) {
        handleCaps();
    } else if (strcmp(cmd, "VERSION") == 0) {
        handleVersion();
    } else if (strcmp(cmd, "GPIO") == 0 || strcmp(cmd, "GPIO_TEST") == 0) {
        handleGpio();
    } else if (strcmp(cmd, "GPIO_SET") == 0) {
        digitalWrite(LED_PIN, HIGH);
        sendResponse("OK");
    } else if (strcmp(cmd, "GPIO_CLR") == 0) {
        digitalWrite(LED_PIN, LOW);
        sendResponse("OK");
    } else if (strcmp(cmd, "SPI_ID") == 0) {
        handleSpiId();
    } else if (strcmp(cmd, "I2C_SCAN") == 0) {
        handleI2cScan();
    } else if (strcmp(cmd, "STATUS") == 0) {
        handleStatus();
    } else if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "?") == 0) {
        handleHelp();
    } else if (strlen(cmd) > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "UNKNOWN:%s", cmd);
        sendResponse(buf);
    }
}

void processChar(char c) {
    if (c == '\r' || c == '\n') {
        if (cmdPos > 0) {
            cmdBuffer[cmdPos] = '\0';
            processCommand(cmdBuffer);
            cmdPos = 0;
        }
    } else if (cmdPos < BUFFER_SIZE - 1) {
        if (c >= 32 && c < 127) {
            cmdBuffer[cmdPos++] = c;
        }
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    ledBlink(3, 100);
    
    SPI.begin();
    pinMode(FLASH_CS, OUTPUT);
    digitalWrite(FLASH_CS, HIGH);
    
    Wire.begin();
    
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    ledBlink(3, 100);
    
    Serial.println();
    Serial.println("=========================================");
    Serial.println("  FiXPro Universal Programmer v2.0.0");
    Serial.println("  RP2040 + Arduino Framework");
    Serial.println("  Web Serial API ready");
    Serial.println("=========================================");
    Serial.println("Commands: PING, CAPS, VERSION, GPIO, SPI_ID, I2C_SCAN, STATUS, HELP");
    Serial.println();
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        processChar(c);
    }
    
    unsigned long now = millis();
    if (now - ledBlinkTime > 1000) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        ledBlinkTime = now;
    }
}
