/**
 * @file usb_protocol.h
 * @brief FiXPro USB Communication Protocol
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @defgroup usb_protocol USB Protocol
 * @{
 *
 * Communication protocol between host and FiXPro device.
 * Uses USB CDC (Virtual COM Port) with custom packet format.
 *
 * Packet Structure:
 * +------+----------+----------+----------+
 * | SYNC | CMD/STAT | LENGTH   | DATA     |
 * +------+----------+----------+----------+
 * | 1B   | 1B       | 2B (LE)  | 0-4096B  |
 * +------+----------+----------+----------+
 *
 * SYNC: 0xAB (packet start marker)
 * CMD: Command code (0x01-0x7F)
 * STAT: Response status (0x80-0xFF for responses)
 * LENGTH: Payload length (little-endian)
 * DATA: Command-specific payload
 */

#ifndef FXPRO_USB_PROTOCOL_H
#define FXPRO_USB_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/async_context_poll.h"

/*============================================================================
 * PROTOCOL CONSTANTS
 *============================================================================*/

#define USB_SYNC_BYTE           0xAB
#define USB_MAX_PACKET_SIZE     4096
#define USB_MIN_PACKET_SIZE     4
#define USB_PACKET_TIMEOUT_MS   1000

/*============================================================================
 * COMMAND CODES (Host -> Device)
 *============================================================================*/

/** @name System Commands
 * @brief Device management commands
 * @{ */
#define CMD_PING                0x01
#define CMD_GET_INFO            0x02
#define CMD_RESET               0x03
#define CMD_GET_STATUS          0x04
#define CMD_SET_DEBUG           0x05
/** @} */

/** @name SPI Commands
 * @brief SPI Flash programming commands
 * @{ */
#define CMD_SPI_INIT            0x10
#define CMD_SPI_DEINIT          0x11
#define CMD_SPI_TRANSFER        0x12
#define CMD_SPI_WRITE           0x13
#define CMD_SPI_READ            0x14
#define CMD_SPI_SET_FREQ        0x15
#define CMD_SPI_SET_MODE        0x16
/** @} */

/** @name I2C Commands
 * @brief I2C EEPROM programming commands
 * @{ */
#define CMD_I2C_INIT            0x20
#define CMD_I2C_DEINIT          0x21
#define CMD_I2C_WRITE           0x22
#define CMD_I2C_READ            0x23
#define CMD_I2C_SCAN            0x24
#define CMD_I2C_SET_FREQ        0x25
/** @} */

/** @name Flash Commands
 * @brief On-chip flash operations (for device firmware updates)
 * @{ */
#define CMD_FLASH_READ          0x30
#define CMD_FLASH_WRITE         0x31
#define CMD_FLASH_ERASE         0x32
#define CMD_FLASH_VERIFY        0x33
/** @} */

/** @name Safety Commands
 * @brief Safety monitoring and configuration
 * @{ */
#define CMD_SAFETY_GET_STATUS   0x40
#define CMD_SAFETY_SET_LIMITS   0x41
#define CMD_SAFETY_READ_VOLTAGE 0x42
#define CMD_SAFETY_READ_CURRENT 0x43
/** @} */

/*============================================================================
 * STATUS CODES (Device -> Host)
 *============================================================================*/

/** @name Success Status
 * @{ */
#define STAT_OK                 0x80
#define STAT_OK_WITH_DATA       0x81
/** @} */

/** @name Error Status
 * @{ */
#define STAT_ERROR              0xE0
#define STAT_ERROR_PARAM        0xE1
#define STAT_ERROR_TIMEOUT      0xE2
#define STAT_ERROR_BUS          0xE3
#define STAT_ERROR_NACK         0xE4
#define STAT_ERROR_NO_DEVICE    0xE5
#define STAT_ERROR_VERIFY       0xE6
#define STAT_ERROR_SAFETY       0xE7
#define STAT_ERROR_BUSY         0xE8
#define STAT_ERROR_UNDERRUN     0xE9
#define STAT_ERROR_OVERRUN      0xEA
/** @} */

/*============================================================================
 * DATA STRUCTURES
 *============================================================================*/

/**
 * @brief USB packet header
 */
typedef struct __attribute__((packed)) {
    uint8_t  sync;       /**< Synchronization byte (0xAB) */
    uint8_t  cmd;        /**< Command or status code */
    uint16_t length;     /**< Payload length (little-endian) */
} usb_packet_header_t;

/**
 * @brief USB packet with flexible payload
 */
typedef struct {
    usb_packet_header_t header;
    uint8_t             payload[USB_MAX_PACKET_SIZE];
} usb_packet_t;

/**
 * @brief Device information structure
 */
typedef struct __attribute__((packed)) {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint8_t  version_patch;
    uint32_t capabilities;    /**< Bitfield of supported features */
    uint8_t  hw_revision;
    char     serial[12];     /**< Device serial number */
    uint32_t flash_size;     /**< On-chip flash size */
    uint32_t sram_size;      /**< SRAM size */
} device_info_t;

/**
 * @brief Device capability flags
 */
typedef enum {
    CAP_SPI         = (1 << 0),
    CAP_I2C         = (1 << 1),
    CAP_JTAG        = (1 << 2),
    CAP_SWD         = (1 << 3),
    CAP_ISP         = (1 << 4),
    CAP_UPDI        = (1 << 5),
    CAP_1WIRE       = (1 << 6),
    CAP_FLASH_READ  = (1 << 8),
    CAP_FLASH_WRITE = (1 << 9),
    CAP_SAFETY      = (1 << 15),
} device_capability_t;

/**
 * @brief SPI configuration parameters
 */
typedef struct __attribute__((packed)) {
    uint32_t frequency;      /**< SPI frequency in Hz */
    uint8_t  mode;           /**< SPI mode (0-3) */
    uint8_t  bits_per_word;  /**< Bits per word (8 or 16) */
    uint8_t  cs_pin;         /**< Chip select pin */
} spi_config_t;

/**
 * @brief I2C configuration parameters
 */
typedef struct __attribute__((packed)) {
    uint32_t frequency;      /**< I2C frequency in Hz */
    uint8_t  address;        /**< I2C device address */
} i2c_config_t;

/**
 * @brief Safety status structure
 */
typedef struct __attribute__((packed)) {
    uint16_t voltage_mv;     /**< Target voltage in millivolts */
    uint16_t current_ma;     /**< Current draw in milliamps */
    uint8_t  temp_celsius;   /**< Temperature in Celsius */
    uint8_t  status;        /**< Safety status flags */
    uint16_t voltage_limit;  /**< Max allowed voltage */
    uint16_t current_limit; /**< Max allowed current */
} safety_status_t;

/*============================================================================
 * FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize USB CDC communication
 * @return true if initialization successful
 */
bool usb_protocol_init(void);

/**
 * @brief Deinitialize USB CDC communication
 */
void usb_protocol_deinit(void);

/**
 * @brief Process incoming USB data
 * @param context Async context for polling
 */
void usb_protocol_poll(async_context_t *context);

/**
 * @brief Send response packet
 * @param status Response status code
 * @param data Pointer to data payload
 * @param len Length of data payload
 * @return true if send successful
 */
bool usb_send_response(uint8_t status, const void *data, uint16_t len);

/**
 * @brief Send simple status response
 * @param status Status code to send
 * @return true if send successful
 */
bool usb_send_status(uint8_t status);

/**
 * @brief Read command packet (blocking)
 * @param packet Pointer to packet buffer
 * @param timeout_ms Timeout in milliseconds
 * @return true if packet received successfully
 */
bool usb_receive_packet(usb_packet_t *packet, uint32_t timeout_ms);

/**
 * @brief Check if USB is connected
 * @return true if USB connected
 */
bool usb_is_connected(void);

/**
 * @brief Get current receive buffer
 * @return Pointer to receive buffer
 */
const uint8_t* usb_get_rx_buffer(void);

/**
 * @brief Get current receive length
 * @return Number of bytes in receive buffer
 */
uint16_t usb_get_rx_length(void);

/*============================================================================
 * COMMAND HANDLERS (to be implemented in protocol handlers)
 *============================================================================*/

/**
 * @brief Handle ping command
 */
void cmd_handle_ping(void);

/**
 * @brief Handle get device info command
 */
void cmd_handle_get_info(void);

/**
 * @brief Handle SPI initialization command
 */
void cmd_handle_spi_init(const spi_config_t *config);

/**
 * @brief Handle SPI transfer command
 */
void cmd_handle_spi_transfer(const uint8_t *data, uint16_t len);

/**
 * @brief Handle SPI read command
 */
void cmd_handle_spi_read(uint32_t address, uint16_t len);

/**
 * @brief Handle SPI write command
 */
void cmd_handle_spi_write(uint32_t address, const uint8_t *data, uint16_t len);

/**
 * @brief Handle I2C initialization command
 */
void cmd_handle_i2c_init(const i2c_config_t *config);

/**
 * @brief Handle I2C write command
 */
void cmd_handle_i2c_write(uint8_t address, const uint8_t *data, uint16_t len);

/**
 * @brief Handle I2C read command
 */
void cmd_handle_i2c_read(uint8_t address, uint16_t len);

/**
 * @brief Handle safety status command
 */
void cmd_handle_safety_status(void);

#endif /* OMNIPROG_USB_PROTOCOL_H */

/** @} */ /* end of usb_protocol group */
