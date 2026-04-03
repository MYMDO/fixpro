/**
 * @file spi_flash.c
 * @brief SPI Flash Memory Driver Implementation
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "spi_flash.h"
#include "../hal/hal.h"
#include "../safety/safety.h"
#include <string.h>

/*============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/

static bool flash_initialized = false;
static flash_info_t current_device;
static jedec_id_t current_jedec;

/**
 * @brief Chip database entry
 */
typedef struct {
    jedec_id_t jedec;
    flash_info_t info;
} chip_entry_t;

/**
 * @brief Known flash chip database
 */
static const chip_entry_t chip_database[] = {
    /* Winbond W25Q series */
    {{0xEF, 0x40, 0x14}, {"Winbond W25Q80",    1*1024*1024,   256, 4096,   65536, 80000000, false, false}},
    {{0xEF, 0x40, 0x15}, {"Winbond W25Q16",    2*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xEF, 0x40, 0x16}, {"Winbond W25Q32",    4*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xEF, 0x40, 0x17}, {"Winbond W25Q64",    8*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xEF, 0x40, 0x18}, {"Winbond W25Q128",  16*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xEF, 0x40, 0x19}, {"Winbond W25Q256",  32*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xEF, 0x60, 0x17}, {"Winbond W25Q64DW",  8*1024*1024,   256, 4096,   65536, 80000000, true,  true}},
    {{0xEF, 0x60, 0x18}, {"Winbond W25Q128DV",16*1024*1024,   256, 4096,   65536, 80000000, true,  true}},
    
    /* Macronix MX25 series */
    {{0xC2, 0x20, 0x15}, {"Macronix MX25L1606",  2*1024*1024,   256, 4096,   65536, 80000000, false, false}},
    {{0xC2, 0x20, 0x16}, {"Macronix MX25L3206",  4*1024*1024,   256, 4096,   65536, 80000000, false, false}},
    {{0xC2, 0x20, 0x17}, {"Macronix MX25L6406",  8*1024*1024,   256, 4096,   65536, 80000000, false, false}},
    {{0xC2, 0x20, 0x18}, {"Macronix MX25L128,", 16*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xC2, 0x26, 0x19}, {"Macronix MX25L256,", 32*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    
    /* Gigadevice GD25Q series */
    {{0xC8, 0x40, 0x14}, {"Gigadevice GD25Q80",  1*1024*1024,   256, 4096,   65536, 80000000, false, false}},
    {{0xC8, 0x40, 0x15}, {"Gigadevice GD25Q16",  2*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xC8, 0x40, 0x16}, {"Gigadevice GD25Q32",  4*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xC8, 0x40, 0x17}, {"Gigadevice GD25Q64",  8*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0xC8, 0x40, 0x18}, {"Gigadevice GD25Q128",16*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    
    /* Micron N25Q series */
    {{0x20, 0xBA, 0x16}, {"Micron N25Q32",      4*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0x20, 0xBA, 0x17}, {"Micron N25Q64",      8*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0x20, 0xBA, 0x18}, {"Micron N25Q128",    16*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    
    /* ISSI IS25LQ series */
    {{0x9D, 0x40, 0x14}, {"ISSI IS25LQ80",      1*1024*1024,   256, 4096,   65536, 80000000, false, false}},
    {{0x9D, 0x40, 0x15}, {"ISSI IS25LQ016",     2*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0x9D, 0x40, 0x16}, {"ISSI IS25LQ032",     4*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0x9D, 0x40, 0x17}, {"ISSI IS25LQ064",     8*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
    {{0x9D, 0x40, 0x18}, {"ISSI IS25LQ128",    16*1024*1024,   256, 4096,   65536, 80000000, true,  false}},
};

#define CHIP_DATABASE_SIZE (sizeof(chip_database) / sizeof(chip_entry_t))

/*============================================================================
 * SPI HARDWARE CONFIGURATION
 *============================================================================*/

/**
 * @brief Default SPI pins for flash programming
 */
#define FLASH_SPI_MOSI    HAL_SPI_MOSI_PIN
#define FLASH_SPI_MISO    HAL_SPI_MISO_PIN
#define FLASH_SPI_SCK     HAL_SPI_SCK_PIN
#define FLASH_SPI_CS      HAL_SPI_CS_PIN

/**
 * @brief Default SPI settings
 */
#define FLASH_SPI_FREQ    10000000    /**< 10 MHz default */
#define FLASH_SPI_MODE    SPI_MODE_0

/*============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/

static void flash_cs_low(void);
static void flash_cs_high(void);
static void flash_tx_byte(uint8_t data);
static uint8_t flash_rx_byte(void);
static void flash_tx_buf(const uint8_t *data, uint16_t len);
static void flash_rx_buf(uint8_t *data, uint16_t len);

/*============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

static void flash_cs_low(void)
{
    hal_spi_cs_select(FLASH_SPI_CS);
}

static void flash_cs_high(void)
{
    hal_spi_cs_release(FLASH_SPI_CS);
}

static void flash_tx_byte(uint8_t data)
{
    hal_spi_write_byte(data);
}

static uint8_t flash_rx_byte(void)
{
    return hal_spi_read_byte();
}

static void flash_tx_buf(const uint8_t *data, uint16_t len)
{
    uint8_t rx_buf[256];
    while (len > 0) {
        uint16_t chunk = len > 256 ? 256 : len;
        hal_spi_transfer(data, rx_buf, chunk);
        data += chunk;
        len -= chunk;
    }
}

static void flash_rx_buf(uint8_t *data, uint16_t len)
{
    uint8_t tx_buf[256];
    memset(tx_buf, 0xFF, sizeof(tx_buf));
    while (len > 0) {
        uint16_t chunk = len > 256 ? 256 : len;
        hal_spi_transfer(tx_buf, data, chunk);
        data += chunk;
        len -= chunk;
    }
}

/*============================================================================
 * PUBLIC API IMPLEMENTATIONS
 *============================================================================*/

bool spi_flash_init(void)
{
    if (flash_initialized) {
        return true;
    }
    
    if (!hal_spi_init(FLASH_SPI_MOSI, FLASH_SPI_MISO, FLASH_SPI_SCK, FLASH_SPI_CS,
                      FLASH_SPI_FREQ, FLASH_SPI_MODE)) {
        return false;
    }
    
    flash_initialized = true;
    return true;
}

void spi_flash_deinit(void)
{
    if (!flash_initialized) {
        return;
    }
    
    hal_spi_deinit();
    flash_initialized = false;
}

bool spi_flash_detect(flash_info_t *info)
{
    if (!spi_flash_read_jedec(&current_jedec)) {
        return false;
    }
    
    if (!spi_flash_lookup_jedec(&current_jedec, &current_device)) {
        memset(info, 0, sizeof(flash_info_t));
        info->jedec_id = current_jedec;
        snprintf(info->name, sizeof(info->name), "Unknown (0x%02X 0x%02X 0x%02X)",
                 current_jedec.manufacturer, current_jedec.memory_type, current_jedec.capacity);
        return false;
    }
    
    if (info) {
        memcpy(info, &current_device, sizeof(flash_info_t));
    }
    
    return true;
}

bool spi_flash_read_jedec(jedec_id_t *jedec)
{
    if (jedec == NULL) {
        return false;
    }
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_READ_ID);
    jedec->manufacturer = flash_rx_byte();
    jedec->memory_type = flash_rx_byte();
    jedec->capacity = flash_rx_byte();
    flash_cs_high();
    
    if (jedec->manufacturer == 0xFF || jedec->manufacturer == 0x00) {
        return false;
    }
    
    return true;
}

uint8_t spi_flash_read_status(void)
{
    uint8_t status;
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_READ_STATUS);
    status = flash_rx_byte();
    flash_cs_high();
    
    return status;
}

void spi_flash_write_status(uint8_t status)
{
    spi_flash_write_enable();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_WRITE_STATUS);
    flash_tx_byte(status);
    flash_cs_high();
    
    spi_flash_wait_ready(100);
}

bool spi_flash_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = hal_get_uptime_ms();
    
    while (spi_flash_read_status() & FLASH_STATUS_BUSY) {
        if (hal_get_uptime_ms() - start > timeout_ms) {
            return false;
        }
    }
    
    return true;
}

void spi_flash_write_enable(void)
{
    flash_cs_low();
    flash_tx_byte(0x06);
    flash_cs_high();
}

void spi_flash_write_disable(void)
{
    flash_cs_low();
    flash_tx_byte(0x04);
    flash_cs_high();
}

bool spi_flash_read(uint32_t address, uint8_t *buffer, uint32_t length)
{
    if (buffer == NULL || length == 0) {
        return false;
    }
    
    if (!spi_flash_wait_ready(100)) {
        return false;
    }
    
    safety_operation_start();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_FAST_READ);
    flash_tx_byte((address >> 16) & 0xFF);
    flash_tx_byte((address >> 8) & 0xFF);
    flash_tx_byte(address & 0xFF);
    flash_tx_byte(0x00);
    
    uint32_t pos = 0;
    while (pos < length) {
        uint32_t chunk = length - pos;
        if (chunk > 256) chunk = 256;
        flash_rx_buf(&buffer[pos], chunk);
        pos += chunk;
    }
    
    flash_cs_high();
    safety_operation_end();
    
    return true;
}

bool spi_flash_page_program(uint32_t address, const uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0) {
        return false;
    }
    
    spi_flash_write_enable();
    
    if (!spi_flash_wait_ready(10)) {
        return false;
    }
    
    safety_operation_start();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_PAGE_PROGRAM);
    flash_tx_byte((address >> 16) & 0xFF);
    flash_tx_byte((address >> 8) & 0xFF);
    flash_tx_byte(address & 0xFF);
    
    uint32_t pos = 0;
    while (pos < length) {
        uint32_t chunk = length - pos;
        if (chunk > 256) chunk = 256;
        flash_tx_buf(&data[pos], chunk);
        pos += chunk;
    }
    
    flash_cs_high();
    
    safety_operation_end();
    
    return spi_flash_wait_ready(500);
}

bool spi_flash_sector_erase(uint32_t address)
{
    spi_flash_write_enable();
    
    if (!spi_flash_wait_ready(10)) {
        return false;
    }
    
    safety_operation_start();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_SECTOR_ERASE);
    flash_tx_byte((address >> 16) & 0xFF);
    flash_tx_byte((address >> 8) & 0xFF);
    flash_tx_byte(address & 0xFF);
    flash_cs_high();
    
    bool result = spi_flash_wait_ready(500);
    
    safety_operation_end();
    
    return result;
}

bool spi_flash_block_erase_32k(uint32_t address)
{
    spi_flash_write_enable();
    
    if (!spi_flash_wait_ready(10)) {
        return false;
    }
    
    safety_operation_start();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_BLOCK_ERASE_32K);
    flash_tx_byte((address >> 16) & 0xFF);
    flash_tx_byte((address >> 8) & 0xFF);
    flash_tx_byte(address & 0xFF);
    flash_cs_high();
    
    bool result = spi_flash_wait_ready(1000);
    
    safety_operation_end();
    
    return result;
}

bool spi_flash_block_erase_64k(uint32_t address)
{
    spi_flash_write_enable();
    
    if (!spi_flash_wait_ready(10)) {
        return false;
    }
    
    safety_operation_start();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_BLOCK_ERASE_64K);
    flash_tx_byte((address >> 16) & 0xFF);
    flash_tx_byte((address >> 8) & 0xFF);
    flash_tx_byte(address & 0xFF);
    flash_cs_high();
    
    bool result = spi_flash_wait_ready(2000);
    
    safety_operation_end();
    
    return result;
}

bool spi_flash_chip_erase(void)
{
    spi_flash_write_enable();
    
    if (!spi_flash_wait_ready(10)) {
        return false;
    }
    
    safety_operation_start();
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_CHIP_ERASE);
    flash_cs_high();
    
    bool result = spi_flash_wait_ready(100000);
    
    safety_operation_end();
    
    return result;
}

bool spi_flash_verify(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint8_t buffer[256];
    
    while (length > 0) {
        uint32_t chunk = length > 256 ? 256 : length;
        
        if (!spi_flash_read(address, buffer, chunk)) {
            return false;
        }
        
        if (memcmp(buffer, data, chunk) != 0) {
            return false;
        }
        
        address += chunk;
        data += chunk;
        length -= chunk;
    }
    
    return true;
}

void spi_flash_erase_suspend(void)
{
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_ERASE_SUSPEND);
    flash_cs_high();
}

void spi_flash_erase_resume(void)
{
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_ERASE_RESUME);
    flash_cs_high();
}

void spi_flash_power_down(void)
{
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_POWER_DOWN);
    flash_cs_high();
}

void spi_flash_power_up(void)
{
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_POWER_UP);
    flash_cs_high();
    
    hal_delay_us(30);
}

void spi_flash_reset(void)
{
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_ENABLE_RESET);
    flash_cs_high();
    
    hal_delay_us(1);
    
    flash_cs_low();
    flash_tx_byte(FLASH_CMD_RESET);
    flash_cs_high();
    
    hal_delay_ms(50);
}

bool spi_flash_lookup_jedec(const jedec_id_t *jedec, flash_info_t *info)
{
    if (jedec == NULL || info == NULL) {
        return false;
    }
    
    for (uint32_t i = 0; i < CHIP_DATABASE_SIZE; i++) {
        const chip_entry_t *entry = &chip_database[i];
        
        if (entry->jedec.manufacturer == jedec->manufacturer &&
            entry->jedec.memory_type == jedec->memory_type &&
            entry->jedec.capacity == jedec->capacity) {
            memcpy(info, &entry->info, sizeof(flash_info_t));
            return true;
        }
    }
    
    return false;
}

const char* spi_flash_get_chip_name(const jedec_id_t *jedec)
{
    static char unknown[] = "Unknown";
    flash_info_t info;
    
    if (spi_flash_lookup_jedec(jedec, &info)) {
        return info.name;
    }
    
    return unknown;
}
