/**
 * @file config.h
 * @brief FiXPro Configuration
 * @version 2.1.0
 */

#ifndef FIXPRO_CONFIG_H
#define FIXPRO_CONFIG_H

#define FIXPRO_VERSION "2.1.0"
#define FIXPRO_NAME "FiXPro"
#define FIXPRO_MANUFACTURER "FiXPro Team"

#define FIXPRO_BOARD "Raspberry Pi Pico / Pico W"
#define FIXPRO_ARCH "RP2040 ARM Cortex-M0+ @ 133MHz"

#define FIXPRO_LED_PIN 25

#define FIXPRO_SPI_MISO 16
#define FIXPRO_SPI_MOSI 19
#define FIXPRO_SPI_SCK 18
#define FIXPRO_SPI_CS 17

#define FIXPRO_I2C_SDA 20
#define FIXPRO_I2C_SCL 21

#define FIXPRO_UART_TX 0
#define FIXPRO_UART_RX 1

#define FIXPRO_BAUDRATE 115200
#define FIXPRO_BUFFER_SIZE 256

#define FIXPRO_CAP_SPI        (1 << 0)
#define FIXPRO_CAP_I2C        (1 << 1)
#define FIXPRO_CAP_GPIO       (1 << 2)
#define FIXPRO_CAP_ISP        (1 << 3)
#define FIXPRO_CAP_SWD        (1 << 4)
#define FIXPRO_CAP_UART       (1 << 5)
#define FIXPRO_CAP_1WIRE      (1 << 6)

#define FIXPRO_CAPABILITIES (FIXPRO_CAP_SPI | FIXPRO_CAP_I2C | FIXPRO_CAP_GPIO)

#define FIXPRO_SPI_MAX_FREQ 10000000
#define FIXPRO_I2C_MAX_FREQ 400000

#define FIXPRO_CMD_TIMEOUT_MS 5000
#define FIXPRO_SPI_TIMEOUT_US 10000

#endif
