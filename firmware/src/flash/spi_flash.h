/**
 * @file spi_flash.h
 * @brief SPI Flash Memory Driver for FiXPro
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @defgroup spi_flash SPI Flash Driver
 * @{
 *
 * Driver for common SPI Flash memory chips including:
 * - Winbond W25Qxx series
 * - Macronix MX25Lxxxx
 * - Micron N25Qxxx
 * - Gigadevice GD25Qxx
 * - and many more via JEDEC ID matching
 */

#ifndef FXPRO_SPI_FLASH_H
#define FXPRO_SPI_FLASH_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * FLASH COMMANDS
 *============================================================================*/

/**
 * @name JEDEC Commands
 * @brief Standard SPI Flash commands (Winbond, Macronix, Micron, etc.)
 * @{ */
#define FLASH_CMD_READ_ID            0x9F    /**< JEDEC ID */
#define FLASH_CMD_READ_STATUS        0x05    /**< Read Status Register */
#define FLASH_CMD_WRITE_STATUS       0x01    /**< Write Status Register */
#define FLASH_CMD_READ_DATA          0x03    /**< Read Data (low speed) */
#define FLASH_CMD_FAST_READ          0x0B    /**< Fast Read */
#define FLASH_CMD_FAST_READ_DUAL     0x3B    /**< Fast Read Dual Output */
#define FLASH_CMD_FAST_READ_QUAD     0x6B    /**< Fast Read Quad Output */
#define FLASH_CMD_PAGE_PROGRAM       0x02    /**< Page Program */
#define FLASH_CMD_QUAD_PAGE_PROGRAM  0x32    /**< Quad Page Program */
#define FLASH_CMD_BLOCK_ERASE_64K    0xD8    /**< Block Erase (64KB) */
#define FLASH_CMD_BLOCK_ERASE_32K    0x52    /**< Block Erase (32KB) */
#define FLASH_CMD_SECTOR_ERASE       0x20    /**< Sector Erase (4KB) */
#define FLASH_CMD_CHIP_ERASE         0xC7    /**< Chip Erase */
#define FLASH_CMD_ERASE_SUSPEND     0x75    /**< Erase Suspend */
#define FLASH_CMD_ERASE_RESUME      0x7A    /**< Erase Resume */
#define FLASH_CMD_POWER_DOWN         0xB9    /**< Power Down */
#define FLASH_CMD_POWER_UP           0xAB    /**< Release Power Down */
#define FLASH_CMD_ENABLE_RESET       0x66    /**< Enable Reset */
#define FLASH_CMD_RESET              0x99    /**< Reset Device */
/** @} */

/**
 * @name Status Register Bits
 * @{ */
#define FLASH_STATUS_BUSY            (1 << 0)  /**< Write in progress */
#define FLASH_STATUS_WEL             (1 << 1)  /**< Write enable latch */
#define FLASH_STATUS_BP0             (1 << 2)  /**< Block protection bit 0 */
#define FLASH_STATUS_BP1             (1 << 3)  /**< Block protection bit 1 */
#define FLASH_STATUS_BP2             (1 << 4)  /**< Block protection bit 2 */
#define FLASH_STATUS_TB              (1 << 5)  /**< Top/Bottom protection */
#define FLASH_STATUS_SEC             (1 << 6)  /**< Sector/Block protection */
#define FLASH_STATUS_SRP0            (1 << 7)  /**< Status register protect 0 */
/** @} */

/*============================================================================
 * DEVICE IDENTIFICATION
 *============================================================================*/

/**
 * @brief JEDEC ID structure
 */
typedef struct {
    uint8_t manufacturer;    /**< Manufacturer ID */
    uint8_t memory_type;     /**< Memory type */
    uint8_t capacity;        /**< Capacity ID */
} jedec_id_t;

/**
 * @brief Flash device information
 */
typedef struct {
    jedec_id_t jedec_id;
    char name[32];
    uint32_t size;           /**< Size in bytes */
    uint32_t page_size;      /**< Page size in bytes */
    uint32_t sector_size;    /**< Sector size in bytes */
    uint32_t block_size;     /**< Block size in bytes */
    uint32_t max_freq;       /**< Maximum frequency in Hz */
    bool supports_quad;      /**< Quad I/O support */
    bool supports_dual;       /**< Dual I/O support */
} flash_info_t;

/*============================================================================
 * FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize SPI Flash driver
 * @return true if initialization successful
 */
bool spi_flash_init(void);

/**
 * @brief Deinitialize SPI Flash driver
 */
void spi_flash_deinit(void);

/**
 * @brief Detect and identify connected flash chip
 * @param info Pointer to store device info
 * @return true if device detected
 */
bool spi_flash_detect(flash_info_t *info);

/**
 * @brief Read JEDEC ID
 * @param jedec Pointer to store JEDEC ID
 * @return true if read successful
 */
bool spi_flash_read_jedec(jedec_id_t *jedec);

/**
 * @brief Read status register
 * @return Status register value
 */
uint8_t spi_flash_read_status(void);

/**
 * @brief Write status register
 * @param status New status value
 */
void spi_flash_write_status(uint8_t status);

/**
 * @brief Wait for busy flag to clear
 * @param timeout_ms Timeout in milliseconds
 * @return true if ready
 */
bool spi_flash_wait_ready(uint32_t timeout_ms);

/**
 * @brief Enable write operations
 */
void spi_flash_write_enable(void);

/**
 * @brief Disable write operations
 */
void spi_flash_write_disable(void);

/**
 * @brief Read data from flash
 * @param address Starting address
 * @param buffer Buffer to store data
 * @param length Number of bytes to read
 * @return true if read successful
 */
bool spi_flash_read(uint32_t address, uint8_t *buffer, uint32_t length);

/**
 * @brief Program page(s)
 * @param address Starting address
 * @param data Data to program
 * @param length Number of bytes to program
 * @return true if program successful
 */
bool spi_flash_page_program(uint32_t address, const uint8_t *data, uint32_t length);

/**
 * @brief Erase sector (4KB)
 * @param address Address in sector to erase
 * @return true if erase successful
 */
bool spi_flash_sector_erase(uint32_t address);

/**
 * @brief Erase 32KB block
 * @param address Address in block to erase
 * @return true if erase successful
 */
bool spi_flash_block_erase_32k(uint32_t address);

/**
 * @brief Erase 64KB block
 * @param address Address in block to erase
 * @return true if erase successful
 */
bool spi_flash_block_erase_64k(uint32_t address);

/**
 * @brief Erase entire chip
 * @return true if erase successful
 */
bool spi_flash_chip_erase(void);

/**
 * @brief Verify flash content
 * @param address Starting address
 * @param data Expected data
 * @param length Number of bytes to verify
 * @return true if content matches
 */
bool spi_flash_verify(uint32_t address, const uint8_t *data, uint32_t length);

/**
 * @brief Suspend ongoing erase operation
 */
void spi_flash_erase_suspend(void);

/**
 * @brief Resume suspended erase operation
 */
void spi_flash_erase_resume(void);

/**
 * @brief Enter power down mode
 */
void spi_flash_power_down(void);

/**
 * @brief Release from power down mode
 */
void spi_flash_power_up(void);

/**
 * @brief Reset flash device
 */
void spi_flash_reset(void);

/*============================================================================
 * CHIP DATABASE LOOKUP
 *============================================================================*/

/**
 * @brief Lookup chip info by JEDEC ID
 * @param jedec JEDEC ID to lookup
 * @param info Pointer to store device info
 * @return true if found in database
 */
bool spi_flash_lookup_jedec(const jedec_id_t *jedec, flash_info_t *info);

/**
 * @brief Get chip name from JEDEC ID
 * @param jedec JEDEC ID
 * @return Chip name string
 */
const char* spi_flash_get_chip_name(const jedec_id_t *jedec);

#endif /* OMNIPROG_SPI_FLASH_H */

/** @} */ /* end of spi_flash group */
