/**
 * @file usb_protocol.c
 * @brief FiXPro USB Communication Protocol Implementation
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "usb_protocol.h"
#include "tusb.h"
#include "pico/async_context_poll.h"
#include "pico/stdlib.h"
#include <string.h>

/*============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/

static async_context_poll_t async_context;
static bool usb_connected = false;
static uint8_t rx_buffer[USB_MAX_PACKET_SIZE];
static uint16_t rx_length = 0;
static uint8_t tx_buffer[USB_MAX_PACKET_SIZE];
static uint16_t tx_length = 0;
static bool response_pending = false;

/*============================================================================
 * TINYUSB CALLBACKS
 *============================================================================*/

/**
 * @brief Invoked when device is mounted (configured)
 */
void tud_mount_cb(void)
{
    usb_connected = true;
    rx_length = 0;
}

/**
 * @brief Invoked when device is unmounted (bus reset/unplugged)
 */
void tud_umount_cb(void)
{
    usb_connected = false;
}

/**
 * @brief Invoked when cdc when line state changed (DTR, RTS)
 */
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;
    
    if (dtr) {
        tud_cdc_write_flush();
    }
}

/**
 * @brief Invoked when new data is received from CDC interface
 */
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf;
    
    while (tud_cdc_available()) {
        uint8_t byte;
        if (tud_cdc_read(&byte, 1) == 1) {
            if (rx_length < USB_MAX_PACKET_SIZE) {
                rx_buffer[rx_length++] = byte;
            }
        }
    }
}

/*============================================================================
 * PACKET PARSING
 *============================================================================*/

/**
 * @brief Find and validate packet in receive buffer
 * @param out_packet Output packet structure
 * @return true if valid packet found
 */
static bool parse_packet(usb_packet_t *out_packet)
{
    uint16_t i;
    
    for (i = 0; i < rx_length; i++) {
        if (rx_buffer[i] == USB_SYNC_BYTE) {
            uint16_t remaining = rx_length - i;
            
            if (remaining < USB_MIN_PACKET_SIZE) {
                continue;
            }
            
            usb_packet_header_t *hdr = (usb_packet_header_t *)&rx_buffer[i];
            
            if (hdr->sync != USB_SYNC_BYTE) {
                continue;
            }
            
            uint16_t payload_len = le16_to_cpu(hdr->length);
            uint16_t total_len = USB_MIN_PACKET_SIZE + payload_len;
            
            if (remaining < total_len) {
                continue;
            }
            
            memcpy(out_packet, &rx_buffer[i], total_len);
            
            memmove(rx_buffer, &rx_buffer[i + total_len], rx_length - i - total_len);
            rx_length -= (i + total_len);
            
            return true;
        }
    }
    
    rx_length = 0;
    return false;
}

/*============================================================================
 * PACKET TRANSMISSION
 *============================================================================*/

/**
 * @brief Send raw data over CDC
 * @param data Data to send
 * @param len Length of data
 * @return true if all data sent
 */
static bool send_raw(const uint8_t *data, uint16_t len)
{
    uint16_t sent = 0;
    uint32_t timeout = 1000;
    
    while (sent < len && timeout--) {
        if (tud_cdc_write_available() > 0) {
            uint16_t chunk = tud_cdc_write(&data[sent], len - sent);
            sent += chunk;
        } else {
            tud_cdc_write_flush();
            sleep_us(10);
        }
        
        if (timeout == 0) {
            return false;
        }
    }
    
    tud_cdc_write_flush();
    return sent == len;
}

bool usb_send_response(uint8_t status, const void *data, uint16_t len)
{
    if (len > USB_MAX_PACKET_SIZE) {
        return false;
    }
    
    usb_packet_header_t header = {
        .sync = USB_SYNC_BYTE,
        .cmd = status,
        .length = cpu_to_le16(len)
    };
    
    if (!send_raw((const uint8_t*)&header, sizeof(header))) {
        return false;
    }
    
    if (len > 0 && data != NULL) {
        return send_raw((const uint8_t*)data, len);
    }
    
    return true;
}

bool usb_send_status(uint8_t status)
{
    return usb_send_response(status, NULL, 0);
}

/*============================================================================
 * PACKET RECEIPTION
 *============================================================================*/

bool usb_receive_packet(usb_packet_t *packet, uint32_t timeout_ms)
{
    absolute_time_t start = get_absolute_time();
    
    while (!parse_packet(packet)) {
        if (usb_timeout_expired(start, timeout_ms)) {
            return false;
        }
        tud_cdc_read_flush();
        sleep_us(100);
    }
    
    return true;
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

bool usb_protocol_init(void)
{
    memset(&async_context, 0, sizeof(async_context));
    async_context_poll_init(&async_context, NULL);
    
    tusb_init();
    
    rx_length = 0;
    tx_length = 0;
    response_pending = false;
    
    return true;
}

void usb_protocol_deinit(void)
{
    async_context_poll_deinit(&async_context);
    tusb_init();
}

/*============================================================================
 * POLLING
 *============================================================================*/

void usb_protocol_poll(async_context_t *context)
{
    (void)context;
    tud_task();
}

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

bool usb_is_connected(void)
{
    return usb_connected && tud_cdc_connected();
}

const uint8_t* usb_get_rx_buffer(void)
{
    return rx_buffer;
}

uint16_t usb_get_rx_length(void)
{
    return rx_length;
}

/*============================================================================
 * COMMAND DISPATCHER
 *============================================================================*/

/**
 * @brief Dispatch received command to appropriate handler
 * @param packet Received packet
 */
void usb_protocol_dispatch(const usb_packet_t *packet)
{
    const uint8_t *payload = packet->payload;
    uint16_t len = le16_to_cpu(packet->length);
    
    switch (packet->header.cmd) {
        case CMD_PING:
            cmd_handle_ping();
            break;
            
        case CMD_GET_INFO:
            cmd_handle_get_info();
            break;
            
        case CMD_RESET:
            usb_send_status(STAT_OK);
            sleep_ms(100);
            reset_usb_boot(0, 0);
            break;
            
        case CMD_GET_STATUS:
            cmd_handle_safety_status();
            break;
            
        case CMD_SPI_INIT:
            if (len >= sizeof(spi_config_t)) {
                cmd_handle_spi_init((const spi_config_t*)payload);
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        case CMD_SPI_TRANSFER:
            if (len > 0) {
                cmd_handle_spi_transfer(payload, len);
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        case CMD_SPI_READ:
            if (len >= 6) {
                uint32_t addr = *((uint32_t*)&payload[0]);
                uint16_t read_len = *((uint16_t*)&payload[4]);
                cmd_handle_spi_read(addr, read_len);
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        case CMD_SPI_WRITE:
            if (len >= 4) {
                uint32_t addr = *((uint32_t*)&payload[0]);
                cmd_handle_spi_write(addr, &payload[4], len - 4);
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        case CMD_I2C_INIT:
            if (len >= sizeof(i2c_config_t)) {
                cmd_handle_i2c_init((const i2c_config_t*)payload);
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        case CMD_I2C_WRITE:
            if (len >= 1) {
                cmd_handle_i2c_write(payload[0], &payload[1], len - 1);
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        case CMD_I2C_READ:
            if (len >= 2) {
                cmd_handle_i2c_read(payload[0], *((uint16_t*)&payload[1]));
            } else {
                usb_send_status(STAT_ERROR_PARAM);
            }
            break;
            
        default:
            usb_send_status(STAT_ERROR_PARAM);
            break;
    }
}

/*============================================================================
 * COMMAND HANDLERS - STUBS (implement in respective modules)
 *============================================================================*/

void cmd_handle_ping(void)
{
    usb_send_status(STAT_OK);
}

void cmd_handle_get_info(void)
{
    device_info_t info = {
        .version_major = 1,
        .version_minor = 0,
        .version_patch = 0,
        .capabilities = CAP_SPI | CAP_I2C | CAP_FLASH_READ | CAP_FLASH_WRITE | CAP_SAFETY,
        .hw_revision = 1,
        .serial = "FXP000001",
        .flash_size = 2048 * 1024,
        .sram_size = 256 * 1024
    };
    usb_send_response(STAT_OK_WITH_DATA, &info, sizeof(info));
}

void cmd_handle_spi_init(const spi_config_t *config)
{
    extern bool spi_hal_init(const spi_config_t *config);
    if (spi_hal_init(config)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR);
    }
}

void cmd_handle_spi_transfer(const uint8_t *data, uint16_t len)
{
    extern uint16_t spi_hal_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len);
    uint8_t rx_buf[USB_MAX_PACKET_SIZE];
    uint16_t received = spi_hal_transfer(data, rx_buf, len);
    usb_send_response(STAT_OK_WITH_DATA, rx_buf, received);
}

void cmd_handle_spi_read(uint32_t address, uint16_t len)
{
    extern bool spi_hal_read(uint32_t addr, uint8_t *buf, uint16_t len);
    uint8_t buf[USB_MAX_PACKET_SIZE];
    
    if (len > USB_MAX_PACKET_SIZE) {
        len = USB_MAX_PACKET_SIZE;
    }
    
    if (spi_hal_read(address, buf, len)) {
        usb_send_response(STAT_OK_WITH_DATA, buf, len);
    } else {
        usb_send_status(STAT_ERROR_BUS);
    }
}

void cmd_handle_spi_write(uint32_t address, const uint8_t *data, uint16_t len)
{
    extern bool spi_hal_write(uint32_t addr, const uint8_t *data, uint16_t len);
    if (spi_hal_write(address, data, len)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR);
    }
}

void cmd_handle_i2c_init(const i2c_config_t *config)
{
    extern bool i2c_hal_init(const i2c_config_t *config);
    if (i2c_hal_init(config)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR);
    }
}

void cmd_handle_i2c_write(uint8_t address, const uint8_t *data, uint16_t len)
{
    extern bool i2c_hal_write(uint8_t addr, const uint8_t *data, uint16_t len);
    if (i2c_hal_write(address, data, len)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR_NACK);
    }
}

void cmd_handle_i2c_read(uint8_t address, uint16_t len)
{
    extern bool i2c_hal_read(uint8_t addr, uint8_t *buf, uint16_t len);
    uint8_t buf[USB_MAX_PACKET_SIZE];
    
    if (len > USB_MAX_PACKET_SIZE) {
        len = USB_MAX_PACKET_SIZE;
    }
    
    if (i2c_hal_read(address, buf, len)) {
        usb_send_response(STAT_OK_WITH_DATA, buf, len);
    } else {
        usb_send_status(STAT_ERROR_NACK);
    }
}

void cmd_handle_safety_status(void)
{
    extern safety_status_t safety_get_status(void);
    safety_status_t status = safety_get_status();
    usb_send_response(STAT_OK_WITH_DATA, &status, sizeof(status));
}
