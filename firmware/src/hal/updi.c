/**
 * @file updi.c
 * @brief UPDI (Unified Programming and Debug Interface) Driver Implementation
 * @author FiXPro Contributors
 * @version 2.0.0
 * @license GPL-3.0
 */

#include "updi.h"
#include "hal.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"
#include "pico/time.h"

#define UPDI_UART        uart1
#define UPDI_UART_TX_PIN 8
#define UPDI_UART_RX_PIN 9

#define UPDI_FREQ        1000000

#define UPDI_SYNC        0x55
#define UPDI_ACK         0x40
#define UPDI_ACK_OK      0x40
#define UPDI_ACK_ERROR   0x49

#define UPDI_LDS_STS     0x00
#define UPDI_STS_KEY      0x20
#define UPDI_STS_CTRLA    0x04
#define UPDI_STS_CTRLB    0x07
#define UPDI_STS_CTRLC    0x08

#define UPDI_CMD_LDCS     0x04
#define UPDI_CMD_STCS     0x05
#define UPDI_CMD_STCK     0x07
#define UPDI_CMD_LD       0x20
#define UPDI_CMD_ST       0x60
#define UPDI_CMD_LD_inc   0x40
#define UPDI_CMD_ST_inc   0x50

#define UPDI_REG_KEY      0x03
#define UPDI_KEY_START    0xE0
#define UPDI_KEY_CHIPERASE 0x20

#define UPDI_CS_STATUS2   0x11

static bool updi_initialized = false;
static uint8_t updi_tx_buf[8];
static uint8_t updi_rx_buf[8];

static void updi_uart_init(uint32_t baud) {
    uart_init(UPDI_UART, baud);
    gpio_set_function(UPDI_UART_TX_PIN, UART_FUNCSEL_NUM(UPDI_UART, UPDI_UART_TX_PIN));
    gpio_set_function(UPDI_UART_RX_PIN, UART_FUNCSEL_NUM(UPDI_UART, UPDI_UART_RX_PIN));
    uart_set_hw_flow(UPDI_UART, false, false);
    uart_set_format(UPDI_UART, 8, 1, UART_PARITY_NONE);
}

static void updi_delay_us(uint32_t us) {
    sleep_us(us);
}

static int updi_uart_read(uint8_t *data, uint32_t timeout_ms) {
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    int idx = 0;
    while (idx < 8) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) < 0) break;
        if (uart_is_readable_within_us(UPDI_UART, 1000)) {
            data[idx++] = uart_getc(UPDI_UART);
        }
    }
    return idx;
}

static void updi_uart_write(const uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uart_putc(UPDI_UART, data[i]);
    }
}

static bool updi_poll(uint8_t expected, uint32_t timeout_ms) {
    uint8_t data;
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        if (uart_is_readable_within_us(UPDI_UART, 1000)) {
            data = uart_getc(UPDI_UART);
            if (data == expected) return true;
        }
        updi_delay_us(100);
    }
    return false;
}

bool hal_updi_init(uint32_t baud) {
    if (updi_initialized) {
        hal_updi_deinit();
    }
    
    gpio_init(UPDI_PIN);
    gpio_set_dir(UPDI_PIN, GPIO_OUT);
    gpio_put(UPDI_PIN, 1);
    updi_delay_us(100);
    
    updi_uart_init(baud > 0 ? baud : UPDI_BAUD_115200);
    
    updi_initialized = true;
    return true;
}

void hal_updi_deinit(void) {
    if (!updi_initialized) return;
    
    uart_deinit(UPDI_UART);
    gpio_deinit(UPDI_PIN);
    updi_initialized = false;
}

bool hal_updi_reset(void) {
    if (!updi_initialized) return false;
    
    updi_uart_write((const uint8_t[]){UPDI_SYNC}, 1);
    updi_delay_us(100);
    
    updi_uart_write((const uint8_t[]){UPDI_CMD_STCS, UPDI_REG_KEY, 0x03}, 3);
    updi_delay_us(1000);
    
    updi_uart_write((const uint8_t[]){UPDI_SYNC}, 1);
    updi_poll(0x55, 1000);
    
    return true;
}

bool hal_updi_poll_ack(void) {
    uint8_t ack;
    absolute_time_t deadline = make_timeout_time_ms(500);
    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        if (uart_is_readable_within_us(UPDI_UART, 5000)) {
            ack = uart_getc(UPDI_UART);
            if (ack == UPDI_ACK_OK) return true;
        }
        updi_delay_us(100);
    }
    return false;
}

bool hal_updi_load_key(const uint8_t *key) {
    if (!updi_initialized || !key) return false;
    
    updi_uart_write((const uint8_t[]){UPDI_CMD_STCS, UPDI_REG_KEY, UPDI_KEY_START | 0x04}, 3);
    updi_delay_us(100);
    
    for (int i = 0; i < 8; i++) {
        updi_uart_write((const uint8_t[]){UPDI_CMD_ST, 0x00, key[i]}, 3);
        updi_delay_us(50);
    }
    
    updi_uart_write((const uint8_t[]){UPDI_SYNC}, 1);
    updi_delay_us(1000);
    
    return true;
}

bool hal_updi_prog_enable(void) {
    updi_uart_write((const uint8_t[]){UPDI_SYNC}, 1);
    updi_delay_us(1000);
    
    uint8_t resp[2];
    updi_uart_write((const uint8_t[]){UPDI_SYNC}, 1);
    updi_uart_write((const uint8_t[]){UPDI_CMD_LDCS, UPDI_CS_STATUS2}, 2);
    updi_delay_us(100);
    
    int len = updi_uart_read(resp, 500);
    
    if (len >= 1) {
        return (resp[0] & 0x04) != 0;
    }
    return false;
}

bool hal_updi_erase_chip(void) {
    if (!updi_initialized) return false;
    
    static const uint8_t key[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    updi_uart_write((const uint8_t[]){UPDI_SYNC}, 1);
    updi_delay_us(100);
    
    hal_updi_load_key(key);
    updi_delay_us(1000);
    
    updi_uart_write((const uint8_t[]){UPDI_CMD_STCS, UPDI_REG_KEY, UPDI_KEY_CHIPERASE | 0x01}, 3);
    updi_delay_us(50000);
    
    hal_updi_reset();
    
    return true;
}

uint8_t hal_updi_read_byte(uint16_t address) {
    if (!updi_initialized) return 0xFF;
    
    updi_tx_buf[0] = UPDI_SYNC;
    updi_tx_buf[1] = UPDI_CMD_LD | (address >> 8);
    updi_tx_buf[2] = UPDI_CMD_LD | (address & 0xFF);
    
    updi_uart_write(updi_tx_buf, 3);
    updi_delay_us(100);
    
    int len = updi_uart_read(updi_rx_buf, 500);
    
    if (len >= 1) {
        return updi_rx_buf[0];
    }
    return 0xFF;
}

bool hal_updi_write_byte(uint16_t address, uint8_t data) {
    if (!updi_initialized) return false;
    
    updi_tx_buf[0] = UPDI_SYNC;
    updi_tx_buf[1] = UPDI_CMD_ST | (address >> 8);
    updi_tx_buf[2] = data;
    
    updi_uart_write(updi_tx_buf, 3);
    
    return hal_updi_poll_ack();
}

bool hal_updi_read_data(uint16_t address, uint8_t *data, uint16_t len) {
    if (!updi_initialized || !data) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        data[i] = hal_updi_read_byte(address + i);
    }
    
    return true;
}

bool hal_updi_write_data(uint16_t address, const uint8_t *data, uint16_t len) {
    if (!updi_initialized || !data) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        if (!hal_updi_write_byte(address + i, data[i])) {
            return false;
        }
    }
    
    return true;
}

bool hal_updi_read_device_info(updi_device_info_t *info) {
    if (!updi_initialized || !info) return false;
    
    info->signature[0] = hal_updi_read_byte(0x1100);
    info->signature[1] = hal_updi_read_byte(0x1101);
    info->signature[2] = hal_updi_read_byte(0x1102);
    
    uint16_t fuses_addr = 0x1280;
    info->fuses = hal_updi_read_byte(fuses_addr);
    
    uint16_t sig_row = 0x1100;
    uint16_t flash_lo = hal_updi_read_byte(0x1108);
    uint16_t flash_hi = hal_updi_read_byte(0x1107);
    info->flash_size = (flash_hi << 8) | flash_lo;
    info->flash_size *= 256;
    
    uint16_t eeprom = hal_updi_read_byte(0x1109);
    info->eeprom_size = eeprom * 32;
    
    return info->signature[0] != 0xFF;
}

uint8_t hal_updi_read_fuse(uint8_t fuse) {
    uint16_t fuse_addr = 0x1280 + fuse;
    return hal_updi_read_byte(fuse_addr);
}

bool hal_updi_write_fuse(uint8_t fuse, uint8_t value) {
    uint16_t fuse_addr = 0x1280 + fuse;
    return hal_updi_write_byte(fuse_addr, value);
}

bool hal_updi_read_flash(uint16_t address, uint8_t *data, uint16_t len) {
    if (!updi_initialized || !data) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        data[i] = hal_updi_read_byte(0x4000 + address + i);
    }
    
    return true;
}

bool hal_updi_write_flash(uint16_t address, const uint8_t *data, uint16_t len) {
    if (!updi_initialized || !data) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t addr = 0x4000 + address + i;
        updi_tx_buf[0] = UPDI_SYNC;
        updi_tx_buf[1] = UPDI_CMD_ST | (addr >> 8);
        updi_tx_buf[2] = data[i];
        updi_uart_write(updi_tx_buf, 3);
        hal_updi_poll_ack();
    }
    
    return true;
}

bool hal_updi_read_eeprom(uint16_t address, uint8_t *data, uint16_t len) {
    if (!updi_initialized || !data) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        data[i] = hal_updi_read_byte(0x1400 + address + i);
    }
    
    return true;
}

bool hal_updi_write_eeprom(uint16_t address, const uint8_t *data, uint16_t len) {
    if (!updi_initialized || !data) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        if (!hal_updi_write_byte(0x1400 + address + i, data[i])) {
            return false;
        }
    }
    
    return true;
}
