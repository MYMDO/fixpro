/**
 * @file main.cpp
 * @brief FiXPro Universal Programmer
 * @version 2.1.0
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "config.h"
#include "commands.h"

static unsigned long ledBlinkTime = 0;
static bool ledState = false;

static void ledBlink(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(FIXPRO_LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(FIXPRO_LED_PIN, LOW);
        delay(delayMs);
    }
}

void setup() {
    pinMode(FIXPRO_LED_PIN, OUTPUT);
    digitalWrite(FIXPRO_LED_PIN, LOW);
    
    ledBlink(3, 100);
    
    SPI.begin();
    pinMode(FIXPRO_SPI_CS, OUTPUT);
    digitalWrite(FIXPRO_SPI_CS, HIGH);
    
    Wire.begin();
    
    Serial.begin(FIXPRO_BAUDRATE);
    while (!Serial) {
        delay(10);
    }
    
    init_commands();
    
    ledBlink(3, 100);
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  FiXPro Universal Programmer v" FIXPRO_VERSION);
    Serial.println("  Board: " FIXPRO_BOARD);
    Serial.println("  Web Serial API ready");
    Serial.println("==========================================");
    Serial.println("Type HELP for commands");
    Serial.println();
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        process_char(c);
    }
    
    unsigned long now = millis();
    if (now - ledBlinkTime > 1000) {
        ledState = !ledState;
        digitalWrite(FIXPRO_LED_PIN, ledState);
        ledBlinkTime = now;
    }
}
