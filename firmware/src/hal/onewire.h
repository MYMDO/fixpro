/**
 * @file onewire.h
 * @brief 1-Wire Protocol Driver
 * @author FiXPro Contributors
 * @version 2.0.0
 * @license GPL-3.0
 */

#ifndef FXPRO_ONEWIRE_H
#define FXPRO_ONEWIRE_H

#include <stdint.h>
#include <stdbool.h>

#define ONEWIRE_PIN         13

#define ONEWIRE_FAMILY_DS18B20  0x28
#define ONEWIRE_FAMILY_DS1822   0x22
#define ONEWIRE_FAMILY_DS1825   0x3B
#define ONEWIRE_FAMILY_DS2431   0x2D
#define ONEWIRE_FAMILY_DS2433   0x26

#define ONEWIRE_CMD_READROM      0x33
#define ONEWIRE_CMD_MATCHROM    0x55
#define ONEWIRE_CMD_SKIPROM     0xCC
#define ONEWIRE_CMD_SEARCH      0xF0
#define ONEWIRE_CMD_ALARMSEARCH 0xEC

#define ONEWIRE_DS18B20_CONVERT     0x44
#define ONEWIRE_DS18B20_WRITESCRATCH 0x4E
#define ONEWIRE_DS18B20_READSCRATCH  0xBE
#define ONEWIRE_DS18B20_COPYSCRATCH 0x48
#define ONEWIRE_DS18B20_RECALLE1    0xB8
#define ONEWIRE_DS18B20_READPOWERSUPPLY 0xB4

typedef struct {
    uint8_t rom[8];
    bool valid;
} onewire_device_t;

typedef struct {
    float temperature;
    bool valid;
    uint8_t scratchpad[9];
} ds18b20_data_t;

bool hal_onewire_init(void);
void hal_onewire_deinit(void);
bool hal_onewire_reset(void);
void hal_onewire_write_bit(bool bit);
bool hal_onewire_read_bit(void);
void hal_onewire_write_byte(uint8_t byte);
uint8_t hal_onewire_read_byte(void);
void hal_onewire_write_bytes(const uint8_t *data, uint16_t len);
void hal_onewire_read_bytes(uint8_t *data, uint16_t len);
bool hal_onewire_search(onewire_device_t *devices, uint8_t max_devices, uint8_t *found);
bool hal_onewire_match_rom(const uint8_t *rom);
bool hal_onewire_skip_rom(void);
bool hal_onewire_read_rom(uint8_t *rom);
bool hal_ds18b20_start_conversion(void);
bool hal_ds18b20_read_scratchpad(uint8_t *rom, uint8_t *scratchpad);
bool hal_ds18b20_read_temperature(uint8_t *rom, ds18b20_data_t *data);
float ds18b20_convert_temperature(const uint8_t *scratchpad);

#endif /* FXPRO_ONEWIRE_H */
