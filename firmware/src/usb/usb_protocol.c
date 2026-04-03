/**
 * @file usb_protocol.c
 * @brief FiXPro USB Communication Protocol Implementation
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "usb_protocol.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/unique_id.h"
#include "../safety/safety.h"
#include "../hal/hal.h"
#include <string.h>

#ifndef le16_to_cpu
#define le16_to_cpu(x) (x)
#endif

#ifndef cpu_to_le16
#define cpu_to_le16(x) (x)
#endif

#ifndef USBD_VID
#define USBD_VID (0x2E8A)
#endif

#ifndef USBD_PID
#define USBD_PID (0x000a)
#endif

#ifndef USBD_MANUFACTURER
#define USBD_MANUFACTURER "FiXPro"
#endif

#ifndef USBD_PRODUCT
#define USBD_PRODUCT "FiXPro Universal Programmer"
#endif

#define USBD_ITF_CDC       (0)
#define USBD_ITF_MAX       (2)

#define USBD_CDC_EP_CMD (0x81)
#define USBD_CDC_EP_OUT (0x02)
#define USBD_CDC_EP_IN (0x82)
#define USBD_CDC_CMD_MAX_SIZE (8)
#define USBD_CDC_IN_OUT_MAX_SIZE (64)

#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
#define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE (0)
#define USBD_MAX_POWER_MA (250)

#define USBD_STR_0 (0x00)
#define USBD_STR_MANUF (0x01)
#define USBD_STR_PRODUCT (0x02)
#define USBD_STR_SERIAL (0x03)
#define USBD_STR_CDC (0x04)

static const tusb_desc_device_t usbd_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0210,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USBD_STR_MANUF,
    .iProduct = USBD_STR_PRODUCT,
    .iSerialNumber = USBD_STR_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t usbd_desc_cfg[USBD_DESC_LEN] = {
    TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_MAX, USBD_STR_0, USBD_DESC_LEN,
        USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE, USBD_MAX_POWER_MA),
    TUD_CDC_DESCRIPTOR(USBD_ITF_CDC, USBD_STR_CDC, USBD_CDC_EP_CMD,
        USBD_CDC_CMD_MAX_SIZE, USBD_CDC_EP_OUT, USBD_CDC_EP_IN, USBD_CDC_IN_OUT_MAX_SIZE),
};

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char *const usbd_desc_str[] = {
    [USBD_STR_MANUF] = USBD_MANUFACTURER,
    [USBD_STR_PRODUCT] = USBD_PRODUCT,
    [USBD_STR_SERIAL] = usbd_serial_str,
    [USBD_STR_CDC] = "FiXPro CDC",
};

const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *)&usbd_desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(__unused uint8_t index) {
    return usbd_desc_cfg;
}

#ifndef USBD_DESC_STR_MAX
#define USBD_DESC_STR_MAX (20)
#endif

const uint16_t *tud_descriptor_string_cb(uint8_t index, __unused uint16_t langid) {
    static uint16_t desc_str[USBD_DESC_STR_MAX];
    
    if (!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }
    
    uint8_t len;
    if (index == 0) {
        desc_str[1] = 0x0409;
        len = 1;
    } else {
        if (index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0])) {
            return NULL;
        }
        const char *str = usbd_desc_str[index];
        for (len = 0; len < USBD_DESC_STR_MAX - 1 && str[len]; ++len) {
            desc_str[1 + len] = str[len];
        }
    }
    
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return desc_str;
}

/*============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/

static bool usb_connected = false;
static uint8_t rx_buffer[USB_MAX_PACKET_SIZE];
static uint16_t rx_length = 0;
static uint16_t tx_length = 0;
static bool response_pending = false;

/*============================================================================
 * TINYUSB CALLBACKS
 *============================================================================*/

void tud_mount_cb(void)
{
    usb_connected = true;
    rx_length = 0;
}

void tud_umount_cb(void)
{
    usb_connected = false;
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;
    
    if (dtr) {
        tud_cdc_write_flush();
    }
}

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
        if (absolute_time_diff_us(start, get_absolute_time()) > (int64_t)(timeout_ms * 1000)) {
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
    tusb_init();
    
    rx_length = 0;
    tx_length = 0;
    response_pending = false;
    
    return true;
}

void usb_protocol_deinit(void)
{
}

void usb_protocol_poll(void)
{
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

void usb_protocol_dispatch(const usb_packet_t *packet)
{
    const uint8_t *payload = packet->payload;
    uint16_t len = le16_to_cpu(packet->header.length);
    
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
 * COMMAND HANDLERS - STUBS
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
    extern bool hal_spi_init(uint32_t mosi, uint32_t miso, uint32_t sck, uint32_t cs, uint32_t frequency, hal_spi_mode_t mode);
    extern void hal_spi_set_mode(hal_spi_mode_t mode);
    if (hal_spi_init(0, 0, 0, config->cs_pin, config->frequency, (hal_spi_mode_t)config->mode)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR);
    }
}

void cmd_handle_spi_transfer(const uint8_t *data, uint16_t len)
{
    extern uint16_t hal_spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t length);
    uint8_t rx_buf[USB_MAX_PACKET_SIZE];
    uint16_t received = hal_spi_transfer(data, rx_buf, len);
    usb_send_response(STAT_OK_WITH_DATA, rx_buf, received);
}

void cmd_handle_spi_read(uint32_t address, uint16_t len)
{
    extern bool spi_flash_read(uint32_t address, uint8_t *buffer, uint32_t length);
    uint8_t buf[USB_MAX_PACKET_SIZE];
    
    if (len > USB_MAX_PACKET_SIZE) {
        len = USB_MAX_PACKET_SIZE;
    }
    
    if (spi_flash_read(address, buf, len)) {
        usb_send_response(STAT_OK_WITH_DATA, buf, len);
    } else {
        usb_send_status(STAT_ERROR_BUS);
    }
}

void cmd_handle_spi_write(uint32_t address, const uint8_t *data, uint16_t len)
{
    extern bool spi_flash_page_program(uint32_t address, const uint8_t *data, uint32_t length);
    if (spi_flash_page_program(address, data, len)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR);
    }
}

void cmd_handle_i2c_init(const i2c_config_t *config)
{
    extern bool hal_i2c_init(uint32_t sda, uint32_t scl, uint32_t frequency);
    if (hal_i2c_init(0, 0, config->frequency)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR);
    }
}

void cmd_handle_i2c_write(uint8_t address, const uint8_t *data, uint16_t len)
{
    extern bool hal_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, uint16_t length);
    if (hal_i2c_write(address, 0, data, len)) {
        usb_send_status(STAT_OK);
    } else {
        usb_send_status(STAT_ERROR_NACK);
    }
}

void cmd_handle_i2c_read(uint8_t address, uint16_t len)
{
    extern bool hal_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length);
    uint8_t buf[USB_MAX_PACKET_SIZE];
    
    if (len > USB_MAX_PACKET_SIZE) {
        len = USB_MAX_PACKET_SIZE;
    }
    
    if (hal_i2c_read(address, 0, buf, len)) {
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
