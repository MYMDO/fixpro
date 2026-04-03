/**
 * @file hal.h
 * @brief Hardware Abstraction Layer for FiXPro
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @defgroup hal Hardware Abstraction Layer
 * @{
 *
 * This module provides hardware abstraction for RP2040-specific functionality:
 * - GPIO pin management
 * - SPI controller
 * - I2C controller
 * - ADC for voltage/current monitoring
 * - PWM for buzzer/LED control
 */

#ifndef FXPRO_HAL_H
#define FXPRO_HAL_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * GPIO CONFIGURATION
 *============================================================================*/

/**
 * @brief SPI pin configuration for FiXPro
 *
 * Default pins (can be changed):
 * - GP0 = MOSI (SPI data out)
 * - GP1 = MISO (SPI data in)
 * - GP2 = SCK  (SPI clock)
 * - GP3 = CS   (Chip select, multiple possible)
 */
#define HAL_SPI_MOSI_PIN    0
#define HAL_SPI_MISO_PIN    1
#define HAL_SPI_SCK_PIN     2
#define HAL_SPI_CS_PIN      3

/**
 * @brief I2C pin configuration
 *
 * Default pins:
 * - GP4 = SDA (I2C data)
 * - GP5 = SCL (I2C clock)
 */
#define HAL_I2C_SDA_PIN     4
#define HAL_I2C_SCL_PIN     5

/**
 * @brief Safety monitoring pins
 *
 * - GP26 = Voltage sense (ADC0)
 * - GP27 = Current sense (ADC1)
 * - GP28 = Temperature sensor (ADC2)
 */
#define HAL_ADC_VSENSE_PIN     26
#define HAL_ADC_CSENSE_PIN     27
#define HAL_ADC_TSENSE_PIN     28

/**
 * @brief Status LED pin
 */
#define HAL_LED_PIN        25

/*============================================================================
 * GPIO FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize GPIO subsystem
 */
void hal_gpio_init(void);

/**
 * @brief Set GPIO pin direction
 * @param pin Pin number
 * @param output true for output, false for input
 */
void hal_gpio_set_dir(uint32_t pin, bool output);

/**
 * @brief Write GPIO pin state
 * @param pin Pin number
 * @param value true for high, false for low
 */
void hal_gpio_put(uint32_t pin, bool value);

/**
 * @brief Read GPIO pin state
 * @param pin Pin number
 * @return Pin state
 */
bool hal_gpio_get(uint32_t pin);

/**
 * @brief Enable pull-up on GPIO pin
 * @param pin Pin number
 */
void hal_gpio_pull_up(uint32_t pin);

/**
 * @brief Enable pull-down on GPIO pin
 * @param pin Pin number
 */
void hal_gpio_pull_down(uint32_t pin);

/**
 * @brief Disable pulls on GPIO pin
 * @param pin Pin number
 */
void hal_gpio_pull_disable(uint32_t pin);

/*============================================================================
 * SPI FUNCTIONS (Hardware SPI + PIO)
 *============================================================================*/

/**
 * @brief SPI operating mode
 */
typedef enum {
    SPI_MODE_0 = 0,  /**< CPOL=0, CPHA=0: Clock idle low, sample on rising */
    SPI_MODE_1 = 1,  /**< CPOL=0, CPHA=1: Clock idle low, sample on falling */
    SPI_MODE_2 = 2,  /**< CPOL=1, CPHA=0: Clock idle high, sample on falling */
    SPI_MODE_3 = 3,  /**< CPOL=1, CPHA=1: Clock idle high, sample on rising */
} hal_spi_mode_t;

/**
 * @brief Initialize SPI peripheral
 * @param mosi MOSI pin
 * @param miso MISO pin
 * @param sck Clock pin
 * @param cs Chip select pin
 * @param frequency Clock frequency in Hz
 * @param mode SPI mode (0-3)
 * @return true if initialization successful
 */
bool hal_spi_init(uint32_t mosi, uint32_t miso, uint32_t sck, uint32_t cs,
                  uint32_t frequency, hal_spi_mode_t mode);

/**
 * @brief Deinitialize SPI peripheral
 */
void hal_spi_deinit(void);

/**
 * @brief Set SPI frequency
 * @param frequency New frequency in Hz
 * @return true if frequency set successfully
 */
bool hal_spi_set_frequency(uint32_t frequency);

/**
 * @brief Set SPI mode
 * @param mode New SPI mode (0-3)
 */
void hal_spi_set_mode(hal_spi_mode_t mode);

/**
 * @brief Transmit and receive data via SPI (full duplex)
 * @param tx_data Transmit buffer
 * @param rx_data Receive buffer
 * @param length Number of bytes
 * @return Number of bytes transferred
 */
uint16_t hal_spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t length);

/**
 * @brief Send byte and ignore response
 * @param data Byte to send
 */
void hal_spi_write_byte(uint8_t data);

/**
 * @brief Receive byte (send 0xFF)
 * @return Received byte
 */
uint8_t hal_spi_read_byte(void);

/**
 * @brief Assert chip select
 * @param cs_pin Chip select pin
 */
void hal_spi_cs_select(uint32_t cs_pin);

/**
 * @brief Deassert chip select
 * @param cs_pin Chip select pin
 */
void hal_spi_cs_release(uint32_t cs_pin);

/*============================================================================
 * I2C FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize I2C peripheral
 * @param sda SDA pin
 * @param scl SCL pin
 * @param frequency Clock frequency in Hz
 * @return true if initialization successful
 */
bool hal_i2c_init(uint32_t sda, uint32_t scl, uint32_t frequency);

/**
 * @brief Deinitialize I2C peripheral
 */
void hal_i2c_deinit(void);

/**
 * @brief Set I2C frequency
 * @param frequency New frequency in Hz
 * @return true if frequency set successfully
 */
bool hal_i2c_set_frequency(uint32_t frequency);

/**
 * @brief Write data to I2C device
 * @param address 7-bit I2C address
 * @param reg Register address (or 0xFF for no register)
 * @param data Data to write
 * @param length Number of bytes
 * @return true if successful
 */
bool hal_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, uint16_t length);

/**
 * @brief Read data from I2C device
 * @param address 7-bit I2C address
 * @param reg Register address (or 0xFF for no register)
 * @param data Buffer for received data
 * @param length Number of bytes to read
 * @return true if successful
 */
bool hal_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length);

/**
 * @brief Scan I2C bus for devices
 * @param addresses Buffer to store found addresses
 * @param max_count Maximum number of addresses to find
 * @return Number of devices found
 */
uint8_t hal_i2c_scan(uint8_t *addresses, uint8_t max_count);

/**
 * @brief Check if I2C device responds
 * @param address 7-bit I2C address
 * @return true if device responds
 */
bool hal_i2c_ping(uint8_t address);

/*============================================================================
 * ADC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize ADC subsystem
 */
void hal_adc_init(void);

/**
 * @brief Read raw ADC value
 * @param channel ADC channel (0-4)
 * @return Raw ADC value (12-bit)
 */
uint16_t hal_adc_read(uint8_t channel);

/**
 * @brief Read voltage in millivolts
 * @param channel ADC channel (0-4)
 * @return Voltage in mV
 */
uint16_t hal_adc_read_voltage_mv(uint8_t channel);

/**
 * @brief Read current in milliamps (requires external sense resistor)
 * @param sense_resistance_mohm Sense resistor in milliohms
 * @return Current in mA
 */
int32_t hal_adc_read_current_ma(uint16_t sense_resistance_mohm);

/**
 * @brief Read chip temperature in Celsius
 * @return Temperature in degrees Celsius
 */
int8_t hal_adc_read_temperature(void);

/*============================================================================
 * PWM/LED FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize status LED
 */
void hal_led_init(void);

/**
 * @brief Set LED brightness
 * @param brightness Brightness 0-255
 */
void hal_led_set_brightness(uint8_t brightness);

/**
 * @brief LED color for status indication
 */
typedef enum {
    LED_OFF = 0,
    LED_RED,
    LED_GREEN,
    LED_BLUE,
    LED_YELLOW,
    LED_CYAN,
    LED_MAGENTA,
    LED_WHITE,
} hal_led_color_t;

/**
 * @brief Set RGB LED color (requires RGB LED hardware)
 * @param color Desired color
 */
void hal_led_set_color(hal_led_color_t color);

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

/**
 * @brief Get system uptime in milliseconds
 * @return Uptime in milliseconds
 */
uint64_t hal_get_uptime_ms(void);

/**
 * @brief Busy wait microseconds
 * @param us Number of microseconds
 */
void hal_delay_us(uint32_t us);

/**
 * @brief Busy wait milliseconds
 * @param ms Number of milliseconds
 */
void hal_delay_ms(uint32_t ms);

/**
 * @brief Get unique chip ID (from OTP fuses)
 * @return 64-bit unique ID
 */
uint64_t hal_get_unique_id(void);

#endif /* OMNIPROG_HAL_H */

/** @} */ /* end of hal group */
