/**
 * @file hal.c
 * @brief Hardware Abstraction Layer Implementation for RP2040
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "hal.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include <string.h>

/*============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/

static bool spi_initialized = false;
static bool i2c_initialized = false;
static bool adc_initialized = false;

static spi_inst_t *current_spi = spi0;
static i2c_inst_t *current_i2c = i2c0;

static uint32_t current_spi_freq = 1000000;
static hal_spi_mode_t current_spi_mode = SPI_MODE_0;

static uint32_t uptime_ms = 0;
static uint32_t pwm_slice_led = 0;
static uint8_t led_brightness = 0;

/*============================================================================
 * GPIO IMPLEMENTATION
 *============================================================================*/

void hal_gpio_init(void)
{
    for (int i = 0; i < 30; i++) {
        gpio_init(i);
    }
}

void hal_gpio_set_dir(uint32_t pin, bool output)
{
    gpio_set_dir(pin, output);
}

void hal_gpio_put(uint32_t pin, bool value)
{
    gpio_put(pin, value);
}

bool hal_gpio_get(uint32_t pin)
{
    return gpio_get(pin);
}

void hal_gpio_pull_up(uint32_t pin)
{
    gpio_pull_up(pin);
}

void hal_gpio_pull_down(uint32_t pin)
{
    gpio_pull_down(pin);
}

void hal_gpio_pull_disable(uint32_t pin)
{
    gpio_disable_pulls(pin);
}

/*============================================================================
 * SPI IMPLEMENTATION
 *============================================================================*/

bool hal_spi_init(uint32_t mosi, uint32_t miso, uint32_t sck, uint32_t cs,
                  uint32_t frequency, hal_spi_mode_t mode)
{
    if (spi_initialized) {
        hal_spi_deinit();
    }
    
    spi_init(current_spi, frequency);
    spi_set_format(current_spi, 8, (spi_cpol_t)(mode & 1), (spi_cpha_t)(mode & 2), SPI_MSB_FIRST);
    
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    
    gpio_init(cs);
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs, true);
    
    spi_initialized = true;
    current_spi_freq = frequency;
    current_spi_mode = mode;
    
    return true;
}

void hal_spi_deinit(void)
{
    if (spi_initialized) {
        spi_deinit(current_spi);
        spi_initialized = false;
    }
}

bool hal_spi_set_frequency(uint32_t frequency)
{
    if (!spi_initialized) return false;
    
    spi_set_baudrate(current_spi, frequency);
    current_spi_freq = frequency;
    return true;
}

void hal_spi_set_mode(hal_spi_mode_t mode)
{
    if (!spi_initialized) return;
    
    spi_set_format(current_spi, 8, (spi_cpol_t)(mode & 1), (spi_cpha_t)(mode & 2), SPI_MSB_FIRST);
    current_spi_mode = mode;
}

uint16_t hal_spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t length)
{
    if (!spi_initialized || length == 0) return 0;
    
    return spi_write_read_blocking(current_spi, tx_data, rx_data, length);
}

void hal_spi_write_byte(uint8_t data)
{
    uint8_t dummy;
    spi_write_read_blocking(current_spi, &data, &dummy, 1);
}

uint8_t hal_spi_read_byte(void)
{
    uint8_t tx = 0xFF;
    uint8_t rx;
    spi_write_read_blocking(current_spi, &tx, &rx, 1);
    return rx;
}

void hal_spi_cs_select(uint32_t cs_pin)
{
    gpio_put(cs_pin, false);
}

void hal_spi_cs_release(uint32_t cs_pin)
{
    gpio_put(cs_pin, true);
}

/*============================================================================
 * I2C IMPLEMENTATION
 *============================================================================*/

bool hal_i2c_init(uint32_t sda, uint32_t scl, uint32_t frequency)
{
    if (i2c_initialized) {
        hal_i2c_deinit();
    }
    
    i2c_init(current_i2c, frequency);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    
    i2c_initialized = true;
    return true;
}

void hal_i2c_deinit(void)
{
    if (i2c_initialized) {
        i2c_deinit(current_i2c);
        i2c_initialized = false;
    }
}

bool hal_i2c_set_frequency(uint32_t frequency)
{
    if (!i2c_initialized) return false;
    
    i2c_set_baudrate(current_i2c, frequency);
    return true;
}

bool hal_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, uint16_t length)
{
    if (!i2c_initialized) return false;
    
    uint8_t buffer[128];
    
    if (reg != 0xFF) {
        if (length + 1 > sizeof(buffer)) return false;
        buffer[0] = reg;
        memcpy(&buffer[1], data, length);
        return i2c_write_blocking(current_i2c, address << 1, buffer, length + 1, false) == (length + 1);
    } else {
        return i2c_write_blocking(current_i2c, address << 1, data, length, false) == length;
    }
}

bool hal_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length)
{
    if (!i2c_initialized) return false;
    
    if (reg != 0xFF) {
        uint8_t reg_byte = reg;
        if (i2c_write_blocking(current_i2c, address << 1, &reg_byte, 1, true) != 1) {
            return false;
        }
    }
    
    int result = i2c_read_blocking(current_i2c, address << 1, data, length, false);
    return result == length;
}

uint8_t hal_i2c_scan(uint8_t *addresses, uint8_t max_count)
{
    if (!i2c_initialized || addresses == NULL) return 0;
    
    uint8_t found = 0;
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        uint8_t dummy;
        int result = i2c_read_blocking(current_i2c, addr << 1, &dummy, 1, false);
        
        if (result >= 0 && found < max_count) {
            addresses[found++] = addr;
        }
    }
    
    return found;
}

bool hal_i2c_ping(uint8_t address)
{
    if (!i2c_initialized) return false;
    
    uint8_t dummy;
    return i2c_read_blocking(current_i2c, address << 1, &dummy, 1, false) >= 0;
}

/*============================================================================
 * ADC IMPLEMENTATION
 *============================================================================*/

void hal_adc_init(void)
{
    if (adc_initialized) return;
    
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_initialized = true;
}

uint16_t hal_adc_read(uint8_t channel)
{
    adc_select_input(channel);
    return adc_read();
}

uint16_t hal_adc_read_voltage_mv(uint8_t channel)
{
    const uint16_t reference_mv = 3300;
    uint16_t raw = hal_adc_read(channel);
    return (raw * reference_mv) / 4095;
}

int32_t hal_adc_read_current_ma(uint16_t sense_resistance_mohm)
{
    uint16_t voltage_mv = hal_adc_read_voltage_mv(HAL_ADC_CSENSE_PIN);
    
    if (sense_resistance_mohm == 0) return 0;
    
    return (voltage_mv * 1000) / sense_resistance_mohm;
}

int8_t hal_adc_read_temperature(void)
{
    adc_select_input(4);
    uint16_t raw = adc_read();
    
    const float conversion = 27.0f - ((raw * 3.3f) / 4095.0f - 0.706f) / 0.001721f;
    return (int8_t)conversion;
}

/*============================================================================
 * LED IMPLEMENTATION
 *============================================================================*/

void hal_led_init(void)
{
    gpio_init(HAL_LED_PIN);
    gpio_set_dir(HAL_LED_PIN, GPIO_OUT);
    
    pwm_slice_led = pwm_gpio_to_slice_num(HAL_LED_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(pwm_slice_led, &config, true);
    pwm_set_gpio_level(HAL_LED_PIN, 0);
}

void hal_led_set_brightness(uint8_t brightness)
{
    led_brightness = brightness;
    uint16_t level = ((uint16_t)brightness << 8) | brightness;
    pwm_set_gpio_level(HAL_LED_PIN, level);
}

void hal_led_set_color(hal_led_color_t color)
{
    uint8_t r = 0, g = 0, b = 0;
    
    switch (color) {
        case LED_RED:     r = 255; break;
        case LED_GREEN:   g = 255; break;
        case LED_BLUE:    b = 255; break;
        case LED_YELLOW:  r = 255; g = 255; break;
        case LED_CYAN:    g = 255; b = 255; break;
        case LED_MAGENTA: r = 255; b = 255; break;
        case LED_WHITE:   r = 255; g = 255; b = 255; break;
        case LED_OFF:
        default:          break;
    }
    
    hal_led_set_brightness((r + g + b) / 3);
}

/*============================================================================
 * UTILITY IMPLEMENTATION
 *============================================================================*/

uint64_t hal_get_uptime_ms(void)
{
    return uptime_ms;
}

void hal_delay_us(uint32_t us)
{
    sleep_us(us);
}

void hal_delay_ms(uint32_t ms)
{
    sleep_ms(ms);
}

uint64_t hal_get_unique_id(void)
{
    volatile uint32_t *id = (volatile uint32_t *)(SIO_BASE + 0x000);
    uint64_t id0 = id[0];
    uint64_t id1 = id[1];
    uint64_t id2 = id[2];
    uint64_t id3 = id[3];
    return (id3 << 96) | (id2 << 64) | (id1 << 32) | id0;
}

void hal_system_reset(void)
{
    reset_usb_boot(0, 0);
}
