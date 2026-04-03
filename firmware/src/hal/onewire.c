/**
 * @file onewire.c
 * @brief 1-Wire Protocol Driver Implementation
 * @author FiXPro Contributors
 * @version 2.0.0
 * @license GPL-3.0
 */

#include "onewire.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#define OW_DELAY_A()    sleep_us(6)
#define OW_DELAY_B()    sleep_us(64)
#define OW_DELAY_C()    sleep_us(60)
#define OW_DELAY_D()    sleep_us(10)
#define OW_DELAY_E()    sleep_us(9)
#define OW_DELAY_F()    sleep_us(55)
#define OW_DELAY_G()    sleep_us(0)
#define OW_DELAY_H()    sleep_us(480)
#define OW_DELAY_I()    sleep_us(70)
#define OW_DELAY_J()    sleep_us(410)

static bool onewire_initialized = false;

static uint8_t onewire_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0x5A;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool hal_onewire_init(void) {
    if (onewire_initialized) {
        hal_onewire_deinit();
    }
    
    gpio_init(ONEWIRE_PIN);
    gpio_set_dir(ONEWIRE_PIN, GPIO_OUT);
    gpio_put(ONEWIRE_PIN, 1);
    
    onewire_initialized = true;
    return true;
}

void hal_onewire_deinit(void) {
    if (!onewire_initialized) return;
    
    gpio_deinit(ONEWIRE_PIN);
    onewire_initialized = false;
}

bool hal_onewire_reset(void) {
    if (!onewire_initialized) return false;
    
    bool presence;
    
    gpio_set_dir(ONEWIRE_PIN, GPIO_OUT);
    gpio_put(ONEWIRE_PIN, 0);
    sleep_us(480);
    
    gpio_set_dir(ONEWIRE_PIN, GPIO_IN);
    sleep_us(70);
    
    presence = !gpio_get(ONEWIRE_PIN);
    
    sleep_us(410);
    
    return presence;
}

void hal_onewire_write_bit(bool bit) {
    if (!onewire_initialized) return;
    
    gpio_set_dir(ONEWIRE_PIN, GPIO_OUT);
    
    if (bit) {
        gpio_put(ONEWIRE_PIN, 0);
        sleep_us(6);
        gpio_put(ONEWIRE_PIN, 1);
        sleep_us(64);
    } else {
        gpio_put(ONEWIRE_PIN, 0);
        sleep_us(60);
        gpio_put(ONEWIRE_PIN, 1);
        sleep_us(10);
    }
}

bool hal_onewire_read_bit(void) {
    if (!onewire_initialized) return false;
    
    bool bit;
    
    gpio_set_dir(ONEWIRE_PIN, GPIO_OUT);
    gpio_put(ONEWIRE_PIN, 0);
    sleep_us(6);
    
    gpio_set_dir(ONEWIRE_PIN, GPIO_IN);
    sleep_us(9);
    
    bit = gpio_get(ONEWIRE_PIN);
    
    sleep_us(55);
    
    return bit;
}

void hal_onewire_write_byte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        hal_onewire_write_bit(byte & 1);
        byte >>= 1;
    }
}

uint8_t hal_onewire_read_byte(void) {
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        if (hal_onewire_read_bit()) {
            byte |= 0x80;
        }
    }
    return byte;
}

void hal_onewire_write_bytes(const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        hal_onewire_write_byte(data[i]);
    }
}

void hal_onewire_read_bytes(uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        data[i] = hal_onewire_read_byte();
    }
}

bool hal_onewire_match_rom(const uint8_t *rom) {
    hal_onewire_write_byte(ONEWIRE_CMD_MATCHROM);
    hal_onewire_write_bytes(rom, 8);
    return true;
}

bool hal_onewire_skip_rom(void) {
    hal_onewire_write_byte(ONEWIRE_CMD_SKIPROM);
    return true;
}

bool hal_onewire_read_rom(uint8_t *rom) {
    hal_onewire_write_byte(ONEWIRE_CMD_READROM);
    hal_onewire_read_bytes(rom, 8);
    return onewire_crc8(rom, 8) == 0;
}

bool hal_onewire_search(onewire_device_t *devices, uint8_t max_devices, uint8_t *found) {
    if (!onewire_initialized || !devices || !found) return false;
    
    uint8_t last_discrepancy = 0;
    uint8_t last_device = 0;
    uint8_t device_num = 0;
    uint8_t rom[8] = {0};
    
    *found = 0;
    
    while (!last_device && device_num < max_devices) {
        if (!hal_onewire_reset()) {
            break;
        }
        
        hal_onewire_write_byte(ONEWIRE_CMD_SEARCH);
        
        uint8_t rom_byte = 0;
        uint8_t rom_byte_mask = 1;
        uint8_t last_zero = 0;
        
        for (uint8_t i = 0; i < 64; i++) {
            bool id_bit = hal_onewire_read_bit();
            bool cmp_id_bit = hal_onewire_read_bit();
            
            if (id_bit && cmp_id_bit) {
                break;
            }
            
            bool search_direction = false;
            
            if (id_bit != cmp_id_bit) {
                search_direction = id_bit;
            } else {
                if (i == last_discrepancy) {
                    search_direction = true;
                } else if (i > last_discrepancy) {
                    search_direction = false;
                } else {
                    search_direction = ((rom[i / 8] & (1 << (i % 8))) != 0);
                }
                
                if (!search_direction) {
                    last_zero = i;
                }
            }
            
            if (search_direction) {
                rom[i / 8] |= (1 << (i % 8));
            } else {
                rom[i / 8] &= ~(1 << (i % 8));
            }
            
            hal_onewire_write_bit(search_direction);
        }
        
        if (onewire_crc8(rom, 8) == 0) {
            for (uint8_t i = 0; i < 8; i++) {
                devices[device_num].rom[i] = rom[i];
            }
            devices[device_num].valid = true;
            device_num++;
        }
        
        last_discrepancy = last_zero;
        if (last_discrepancy == 0) {
            last_device = 1;
        }
    }
    
    *found = device_num;
    return device_num > 0;
}

bool hal_ds18b20_start_conversion(void) {
    if (!hal_onewire_reset()) return false;
    
    hal_onewire_skip_rom();
    hal_onewire_write_byte(ONEWIRE_DS18B20_CONVERT);
    
    return true;
}

bool hal_ds18b20_read_scratchpad(uint8_t *rom, uint8_t *scratchpad) {
    if (!hal_onewire_reset()) return false;
    
    if (rom) {
        hal_onewire_match_rom(rom);
    } else {
        hal_onewire_skip_rom();
    }
    
    hal_onewire_write_byte(ONEWIRE_DS18B20_READSCRATCH);
    hal_onewire_read_bytes(scratchpad, 9);
    
    return onewire_crc8(scratchpad, 9) == 0;
}

float ds18b20_convert_temperature(const uint8_t *scratchpad) {
    int16_t raw = ((int16_t)scratchpad[1] << 8) | scratchpad[0];
    
    if (scratchpad[4] & 0x80) {
        raw = ~raw + 1;
    }
    
    return (float)raw / 16.0f;
}

bool hal_ds18b20_read_temperature(uint8_t *rom, ds18b20_data_t *data) {
    if (!data) return false;
    
    if (!hal_onewire_reset()) {
        data->valid = false;
        return false;
    }
    
    if (rom) {
        hal_onewire_match_rom(rom);
    } else {
        hal_onewire_skip_rom();
    }
    
    hal_onewire_write_byte(ONEWIRE_DS18B20_CONVERT);
    
    sleep_us(750000);
    
    if (!hal_ds18b20_read_scratchpad(rom, data->scratchpad)) {
        data->valid = false;
        return false;
    }
    
    data->temperature = ds18b20_convert_temperature(data->scratchpad);
    data->valid = true;
    
    return true;
}
