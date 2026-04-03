/**
 * @file updi.h
 * @brief UPDI (Unified Programming and Debug Interface) Driver
 * @author FiXPro Contributors
 * @version 2.0.0
 * @license GPL-3.0
 */

#ifndef FXPRO_UPDI_H
#define FXPRO_UPDI_H

#include <stdint.h>
#include <stdbool.h>

#define UPDI_PIN            12

#define UPDI_BAUD_115200    115200
#define UPDI_BAUD_230400    230400
#define UPDI_BAUD_460800    460800

#define UPDI_MAX_FUSE       32
#define UPDI_MAX_LOCKBITS   1

typedef enum {
    UPDI_OK = 0,
    UPDI_ERROR_INIT,
    UPDI_ERROR_TIMEOUT,
    UPDI_ERROR_NACK,
    UPDI_ERROR_VERIFY,
    UPDI_ERROR_CHIP,
} updi_error_t;

typedef struct {
    uint8_t signature[3];
    uint8_t revision;
    uint16_t flash_size;
    uint16_t eeprom_size;
    uint16_t fuses;
    uint16_t lockbits;
} updi_device_info_t;

typedef enum {
    UPDI_KEY_64 = 0xE0,
    UPDI_KEY_128 = 0xE1,
    UPDI_KEY_CHIPERASE = 0x80,
} updi_key_t;

bool hal_updi_init(uint32_t baud);
void hal_updi_deinit(void);
bool hal_updi_reset(void);
bool hal_updi_poll_ack(void);
uint8_t hal_updi_read_byte(uint16_t address);
bool hal_updi_write_byte(uint16_t address, uint8_t data);
bool hal_updi_read_data(uint16_t address, uint8_t *data, uint16_t len);
bool hal_updi_write_data(uint16_t address, const uint8_t *data, uint16_t len);
bool hal_updi_load_key(const uint8_t *key);
bool hal_updi_prog_enable(void);
bool hal_updi_erase_chip(void);
bool hal_updi_read_device_info(updi_device_info_t *info);
bool hal_updi_write_fuse(uint8_t fuse, uint8_t value);
uint8_t hal_updi_read_fuse(uint8_t fuse);
bool hal_updi_write_flash(uint16_t address, const uint8_t *data, uint16_t len);
bool hal_updi_read_flash(uint16_t address, uint8_t *data, uint16_t len);
bool hal_updi_write_eeprom(uint16_t address, const uint8_t *data, uint16_t len);
bool hal_updi_read_eeprom(uint16_t address, uint8_t *data, uint16_t len);

#endif /* FXPRO_UPDI_H */
