/**
 * @file jtag.c
 * @brief JTAG Hardware Abstraction Layer Implementation
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "jtag.h"
#include "hal.h"
#include "hardware/gpio.h"

static bool jtag_initialized = false;



bool hal_jtag_init(void)
{
    if (jtag_initialized) {
        hal_jtag_deinit();
    }
    
    gpio_init(JTAG_TCK_PIN);
    gpio_init(JTAG_TMS_PIN);
    gpio_init(JTAG_TDI_PIN);
    gpio_init(JTAG_TDO_PIN);
    
    gpio_set_dir(JTAG_TCK_PIN, GPIO_OUT);
    gpio_set_dir(JTAG_TMS_PIN, GPIO_OUT);
    gpio_set_dir(JTAG_TDI_PIN, GPIO_OUT);
    gpio_set_dir(JTAG_TDO_PIN, GPIO_IN);
    
    gpio_put(JTAG_TCK_PIN, 0);
    gpio_put(JTAG_TMS_PIN, 0);
    gpio_put(JTAG_TDI_PIN, 0);
    
    gpio_put(JTAG_TCK_PIN, 1);
    hal_delay_us(1);
    gpio_put(JTAG_TCK_PIN, 0);
    
    jtag_initialized = true;
    return true;
}

void hal_jtag_deinit(void)
{
    if (!jtag_initialized) return;
    
    gpio_deinit(JTAG_TCK_PIN);
    gpio_deinit(JTAG_TMS_PIN);
    gpio_deinit(JTAG_TDI_PIN);
    gpio_deinit(JTAG_TDO_PIN);
    
    jtag_initialized = false;
}

void hal_jtag_reset(void)
{
    gpio_put(JTAG_TMS_PIN, 1);
    
    for (int i = 0; i < 6; i++) {
        gpio_put(JTAG_TCK_PIN, 1);
        hal_delay_us(1);
        gpio_put(JTAG_TCK_PIN, 0);
        hal_delay_us(1);
    }
    
    gpio_put(JTAG_TMS_PIN, 0);
}

bool hal_jtag_shift(const uint8_t *tdi_data, uint16_t num_bits, uint8_t *tdo_data)
{
    if (!jtag_initialized || num_bits == 0) return false;
    
    uint16_t bit_index = 0;
    uint8_t current_byte = 0;
    uint8_t tdo_byte = 0;
    
    while (bit_index < num_bits) {
        bool tdi_bit = (tdi_data[bit_index / 8] >> (7 - (bit_index % 8))) & 1;
        
        gpio_put(JTAG_TDI_PIN, tdi_bit);
        
        bool last_bit = (bit_index == num_bits - 1);
        if (last_bit) {
            gpio_put(JTAG_TMS_PIN, 1);
        }
        
        gpio_put(JTAG_TCK_PIN, 1);
        hal_delay_us(1);
        
        bool tdo_bit = gpio_get(JTAG_TDO_PIN);
        gpio_put(JTAG_TCK_PIN, 0);
        hal_delay_us(1);
        
        gpio_put(JTAG_TMS_PIN, 0);
        
        tdo_byte = (tdo_byte << 1) | (tdo_bit ? 1 : 0);
        
        if ((bit_index % 8) == 7) {
            if (tdo_data) {
                tdo_data[bit_index / 8] = tdo_byte;
            }
            tdo_byte = 0;
        }
        
        bit_index++;
    }
    
    if ((num_bits % 8) != 0 && tdo_data) {
        tdo_byte <<= (8 - (num_bits % 8));
        tdo_data[num_bits / 8] = tdo_byte;
    }
    
    return true;
}

uint32_t hal_jtag_read_idcode(void)
{
    if (!jtag_initialized) return 0;
    
    hal_jtag_reset();
    
    gpio_put(JTAG_TMS_PIN, 0);
    hal_delay_us(1);
    
    for (int i = 0; i < 32; i++) {
        gpio_put(JTAG_TDI_PIN, (i < 31) ? 1 : 0);
        
        gpio_put(JTAG_TCK_PIN, 1);
        hal_delay_us(1);
        
        gpio_put(JTAG_TCK_PIN, 0);
        hal_delay_us(1);
    }
    
    uint8_t tdo_data[4] = {0};
    hal_jtag_shift((const uint8_t[]){0x00, 0x00, 0x00, 0x01}, 32, tdo_data);
    
    return (uint32_t)tdo_data[0] | 
           ((uint32_t)tdo_data[1] << 8) | 
           ((uint32_t)tdo_data[2] << 16) | 
           ((uint32_t)tdo_data[3] << 24);
}

bool hal_jtag_is_initialized(void)
{
    return jtag_initialized;
}
