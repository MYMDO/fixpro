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

/** @name JTAG Commands
 * @brief JTAG debugging commands
 * @{ */
#define CMD_JTAG_INIT           0x30
#define CMD_JTAG_DEINIT         0x31
#define CMD_JTAG_RESET          0x32
#define CMD_JTAG_SHIFT          0x33
#define CMD_JTAG_READ_IDCODE    0x34
#define CMD_JTAG_SET_FREQ       0x35
/** @} */

/** @name SWD Commands
 * @brief SWD (Serial Wire Debug) commands
 * @{ */
#define CMD_SWD_INIT            0x40
#define CMD_SWD_DEINIT          0x41
#define CMD_SWD_RESET           0x42
#define CMD_SWD_READ            0x43
#define CMD_SWD_WRITE           0x44
#define CMD_SWD_CONNECT         0x45
#define CMD_SWD_READ_DPBANKSEL  0x46
#define CMD_SWD_WRITE_DPBANKSEL 0x47
/** @} */

/** @name Flash Commands
 * @brief On-chip flash operations (for device firmware updates)
 * @{ */
#define CMD_FLASH_READ          0x50
#define CMD_FLASH_WRITE         0x51
#define CMD_FLASH_ERASE         0x52
#define CMD_FLASH_VERIFY        0x53
/** @} */

/** @name Safety Commands
 * @brief Safety monitoring and configuration
 * @{ */
#define CMD_SAFETY_GET_STATUS   0x60
#define CMD_SAFETY_SET_LIMITS   0x61
#define CMD_SAFETY_READ_VOLTAGE 0x62
#define CMD_SAFETY_READ_CURRENT 0x63
/** @} */

/** @name UPDI Commands
 * @brief AVR UPDI programming commands
 * @{ */
#define CMD_UPDI_INIT           0x70
#define CMD_UPDI_DEINIT         0x71
#define CMD_UPDI_RESET          0x72
#define CMD_UPDI_READ_INFO      0x73
#define CMD_UPDI_READ_FLASH     0x74
#define CMD_UPDI_WRITE_FLASH    0x75
#define CMD_UPDI_READ_EEPROM    0x76
#define CMD_UPDI_WRITE_EEPROM   0x77
#define CMD_UPDI_READ_FUSE      0x78
#define CMD_UPDI_WRITE_FUSE     0x79
#define CMD_UPDI_ERASE_CHIP     0x7A
/** @} */

/** @name 1-Wire Commands
 * @brief 1-Wire devices (DS18B20, etc.)
 * @{ */
#define CMD_1WIRE_INIT          0x80
#define CMD_1WIRE_DEINIT        0x81
#define CMD_1WIRE_RESET         0x82
#define CMD_1WIRE_SEARCH        0x83
#define CMD_1WIRE_READ_ROM     0x84
#define CMD_1WIRE_READ_TEMP     0x85
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
 */
void usb_protocol_poll(void);

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

/**
 * @brief Dispatch command packet to handler
 * @param packet Pointer to received packet
 */
void usb_protocol_dispatch(const usb_packet_t *packet);

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
 * @brief Handle JTAG initialization command
 */
void cmd_handle_jtag_init(void);

/**
 * @brief Handle JTAG deinitialization command
 */
void cmd_handle_jtag_deinit(void);

/**
 * @brief Handle JTAG reset command
 */
void cmd_handle_jtag_reset(void);

/**
 * @brief Handle JTAG shift command
 * @param num_bits Number of bits to shift
 * @param data TDI data to shift out
 * @param len Length of data buffer
 */
void cmd_handle_jtag_shift(uint16_t num_bits, const uint8_t *data, uint16_t len);

/**
 * @brief Handle JTAG read IDCODE command
 */
void cmd_handle_jtag_read_idcode(void);

/**
 * @brief Handle SWD initialization command
 */
void cmd_handle_swd_init(void);

/**
 * @brief Handle SWD deinitialization command
 */
void cmd_handle_swd_deinit(void);

/**
 * @brief Handle SWD reset command
 */
void cmd_handle_swd_reset(void);

/**
 * @brief Handle SWD read command
 * @param address SWD register address
 */
void cmd_handle_swd_read(uint8_t address);

/**
 * @brief Handle SWD write command
 * @param address SWD register address
 * @param value 32-bit value to write
 */
void cmd_handle_swd_write(uint8_t address, uint32_t value);

/**
 * @brief Handle safety status command
 */
void cmd_handle_safety_status(void);

/*============================================================================
 * UPDI COMMAND HANDLERS
 *============================================================================*/

/**
 * @brief Handle UPDI initialization command
 * @param baud UART baud rate
 */
void cmd_handle_updi_init(uint32_t baud);

/**
 * @brief Handle UPDI deinitialization command
 */
void cmd_handle_updi_deinit(void);

/**
 * @brief Handle UPDI reset command
 */
void cmd_handle_updi_reset(void);

/**
 * @brief Handle UPDI read device info command
 */
void cmd_handle_updi_read_info(void);

/**
 * @brief Handle UPDI read flash command
 * @param address Flash address
 * @param len Number of bytes to read
 */
void cmd_handle_updi_read_flash(uint16_t address, uint16_t len);

/**
 * @brief Handle UPDI write flash command
 * @param address Flash address
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 */
void cmd_handle_updi_write_flash(uint16_t address, const uint8_t *data, uint16_t len);

/**
 * @brief Handle UPDI read EEPROM command
 * @param address EEPROM address
 * @param len Number of bytes to read
 */
void cmd_handle_updi_read_eeprom(uint16_t address, uint16_t len);

/**
 * @brief Handle UPDI write EEPROM command
 * @param address EEPROM address
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 */
void cmd_handle_updi_write_eeprom(uint16_t address, const uint8_t *data, uint16_t len);

/**
 * @brief Handle UPDI read fuse command
 * @param fuse Fuse number
 */
void cmd_handle_updi_read_fuse(uint8_t fuse);

/**
 * @brief Handle UPDI write fuse command
 * @param fuse Fuse number
 * @param value Fuse value
 */
void cmd_handle_updi_write_fuse(uint8_t fuse, uint8_t value);

/**
 * @brief Handle UPDI chip erase command
 */
void cmd_handle_updi_erase_chip(void);

/*============================================================================
 * 1-WIRE COMMAND HANDLERS
 *============================================================================*/

/**
 * @brief Handle 1-Wire initialization command
 */
void cmd_handle_onewire_init(void);

/**
 * @brief Handle 1-Wire deinitialization command
 */
void cmd_handle_onewire_deinit(void);

/**
 * @brief Handle 1-Wire reset command
 */
void cmd_handle_onewire_reset(void);

/**
 * @brief Handle 1-Wire search devices command
 * @param max_devices Maximum number of devices to find
 */
void cmd_handle_onewire_search(uint8_t max_devices);

/**
 * @brief Handle 1-Wire read ROM command
 */
void cmd_handle_onewire_read_rom(void);

/**
 * @brief Handle 1-Wire read temperature command
 * @param has_rom true if ROM is provided, false for skip ROM
 * @param rom ROM code (8 bytes, only used if has_rom is true)
 */
void cmd_handle_onewire_read_temp(bool has_rom, const uint8_t *rom);

#endif /* OMNIPROG_USB_PROTOCOL_H */

/** @} */ /* end of usb_protocol group */
