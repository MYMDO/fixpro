/**
 * @file swd.c
 * @brief SWD (Serial Wire Debug) Hardware Abstraction Layer Implementation
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "swd.h"
#include "hal.h"
#include "hardware/gpio.h"
#include <string.h>

#define SWD_DELAY() do { asm volatile ("nop\nnop\nnop\nnop\n"); } while(0)

static bool swd_initialized = false;

static void swd_clock_cycle(void)
{
    gpio_put(SWD_CLK_PIN, 1);
    SWD_DELAY();
    gpio_put(SWD_CLK_PIN, 0);
    SWD_DELAY();
}

static void swd_write_bit(bool bit)
{
    gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
    gpio_put(SWD_DIO_PIN, bit ? 1 : 0);
    swd_clock_cycle();
}

static bool swd_read_bit(void)
{
    gpio_set_dir(SWD_DIO_PIN, GPIO_IN);
    swd_clock_cycle();
    return gpio_get(SWD_DIO_PIN) != 0;
}

static uint8_t swd_read_bits(int count)
{
    uint8_t value = 0;
    gpio_set_dir(SWD_DIO_PIN, GPIO_IN);
    for (int i = count - 1; i >= 0; i--) {
        value |= swd_read_bit() ? (1 << i) : 0;
    }
    return value;
}

static void swd_write_bits(uint32_t value, int count)
{
    gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
    for (int i = count - 1; i >= 0; i--) {
        gpio_put(SWD_DIO_PIN, (value >> i) & 1 ? 1 : 0);
        swd_clock_cycle();
    }
}

bool hal_swd_init(void)
{
    if (swd_initialized) {
        hal_swd_deinit();
    }
    
    gpio_init(SWD_CLK_PIN);
    gpio_init(SWD_DIO_PIN);
    
    gpio_set_dir(SWD_CLK_PIN, GPIO_OUT);
    gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
    
    gpio_put(SWD_CLK_PIN, 0);
    gpio_put(SWD_DIO_PIN, 0);
    
    swd_initialized = true;
    return true;
}

void hal_swd_deinit(void)
{
    if (!swd_initialized) return;
    
    gpio_deinit(SWD_CLK_PIN);
    gpio_deinit(SWD_DIO_PIN);
    
    swd_initialized = false;
}

void hal_swd_reset(void)
{
    gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
    gpio_put(SWD_DIO_PIN, 1);
    
    for (int i = 0; i < 60; i++) {
        swd_clock_cycle();
    }
    
    gpio_put(SWD_DIO_PIN, 0);
}

static bool swd_wait_idle(void)
{
    gpio_set_dir(SWD_DIO_PIN, GPIO_IN);
    
    for (int i = 0; i < 100; i++) {
        if (!gpio_get(SWD_DIO_PIN)) {
            return true;
        }
        swd_clock_cycle();
    }
    
    return false;
}

static uint8_t swd_transfer(uint8_t request, uint32_t data, int data_bits)
{
    swd_write_bits(request, 8);
    
    uint8_t ack = swd_read_bits(3);
    
    if (ack == SWD_ACK_OK) {
        if (data_bits > 0) {
            if (request & (1 << 5)) {
                uint32_t read_val = 0;
                for (int i = data_bits - 1; i >= 0; i--) {
                    gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
                    gpio_put(SWD_DIO_PIN, (data >> i) & 1 ? 1 : 0);
                    swd_clock_cycle();
                }
                gpio_set_dir(SWD_DIO_PIN, GPIO_IN);
                swd_clock_cycle();
            } else {
                uint32_t rd = swd_read_bits(data_bits);
                data = rd;
            }
        } else if (request & (1 << 5)) {
            swd_wait_idle();
        }
        
        uint8_t parity = swd_read_bits(1);
        swd_clock_cycle();
    }
    
    return ack;
}

bool hal_swd_read(uint8_t apnzp, uint8_t reg, uint32_t *value)
{
    if (!swd_initialized || value == NULL) return false;
    
    uint8_t request = 0x81 | (apnzp << 0) | ((reg & 0x0C) << 1);
    uint8_t ack = swd_transfer(request, 0, 32);
    
    if (ack == SWD_ACK_OK) {
        *value = 0;
        gpio_set_dir(SWD_DIO_PIN, GPIO_IN);
        for (int i = 31; i >= 0; i--) {
            swd_clock_cycle();
            if (gpio_get(SWD_DIO_PIN)) {
                *value |= (1 << i);
            }
        }
        
        swd_clock_cycle();
        swd_clock_cycle();
        
        return true;
    }
    
    return false;
}

bool hal_swd_write(uint8_t apnzp, uint8_t reg, uint32_t value)
{
    if (!swd_initialized) return false;
    
    swd_wait_idle();
    
    uint8_t request = 0x81 | (apnzp << 0) | ((reg & 0x0C) << 1) | (1 << 4);
    
    for (int i = 31; i >= 0; i--) {
        gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
        gpio_put(SWD_DIO_PIN, (value >> i) & 1 ? 1 : 0);
        swd_clock_cycle();
    }
    
    uint8_t parity = 0;
    for (int i = 0; i < 32; i++) {
        parity ^= (value >> i) & 1;
    }
    gpio_set_dir(SWD_DIO_PIN, GPIO_OUT);
    gpio_put(SWD_DIO_PIN, parity ? 1 : 0);
    swd_clock_cycle();
    
    gpio_set_dir(SWD_DIO_PIN, GPIO_IN);
    swd_clock_cycle();
    swd_clock_cycle();
    
    return true;
}

bool hal_swd_read_dp(uint8_t reg, uint32_t *value)
{
    return hal_swd_read(0, reg, value);
}

bool hal_swd_read_ap(uint8_t reg, uint32_t *value)
{
    hal_swd_write(0, 0x08, (reg & 0xF0) << 8);
    return hal_swd_read(1, reg, value);
}

bool hal_swd_write_ap(uint8_t reg, uint32_t value)
{
    hal_swd_write(0, 0x08, (reg & 0xF0) << 8);
    return hal_swd_write(1, reg, value);
}

bool hal_swd_connect(void)
{
    hal_swd_reset();
    
    hal_swd_write(0, 0x04, 0x50000000);
    
    uint32_t idcode;
    if (hal_swd_read_dp(SWD_DP_REG_IDCODE, &idcode)) {
        return idcode != 0xFFFFFFFF && idcode != 0;
    }
    
    return false;
}

bool hal_swd_is_initialized(void)
{
    return swd_initialized;
}
