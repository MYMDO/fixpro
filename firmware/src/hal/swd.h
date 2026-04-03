/**
 * @file swd.h
 * @brief SWD (Serial Wire Debug) Hardware Abstraction Layer
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @defgroup swd SWD Interface
 * @{
 */

#ifndef FXPRO_SWD_H
#define FXPRO_SWD_H

#include <stdint.h>
#include <stdbool.h>

#define SWD_CLK_PIN   10
#define SWD_DIO_PIN   11

#define SWD_DP_REG_IDCODE    0x00
#define SWD_DP_REG_ABORT     0x00
#define SWD_DP_REG_CTRL      0x04
#define SWD_DP_REG_STAT      0x04
#define SWD_DP_REG_SELECT    0x08
#define SWD_DP_REG_RDBUFF    0x0C
#define SWD_AP_REG_IDR       0xFC

#define SWD_ACK_OK          1
#define SWD_ACK_WAIT        2
#define SWD_ACK_FAULT       4
#define SWD_ACK_NO_ACK      7

/**
 * @brief Initialize SWD interface
 * @return true if initialization successful
 */
bool hal_swd_init(void);

/**
 * @brief Deinitialize SWD interface
 */
void hal_swd_deinit(void);

/**
 * @brief Perform SWD line reset (50+ clocks with SWDIO high)
 */
void hal_swd_reset(void);

/**
 * @brief Read from SWD register
 * @param apnzp APnDP bit (0=DP, 1=AP)
 * @param reg Register address (0-15)
 * @param value Pointer to store read value
 * @return true if successful
 */
bool hal_swd_read(uint8_t apnzp, uint8_t reg, uint32_t *value);

/**
 * @brief Write to SWD register
 * @param apnzp APnDP bit (0=DP, 1=AP)
 * @param reg Register address (0-15)
 * @param value Value to write
 * @return true if successful
 */
bool hal_swd_write(uint8_t apnzp, uint8_t reg, uint32_t value);

/**
 * @brief Read DP register (convenience function)
 * @param reg Register address (0-15)
 * @param value Pointer to store read value
 * @return true if successful
 */
bool hal_swd_read_dp(uint8_t reg, uint32_t *value);

/**
 * @brief Read AP register (convenience function)
 * @param reg Register address (0-15)
 * @param value Pointer to store read value
 * @return true if successful
 */
bool hal_swd_read_ap(uint8_t reg, uint32_t *value);

/**
 * @brief Write AP register (convenience function)
 * @param reg Register address (0-15)
 * @param value Value to write
 * @return true if successful
 */
bool hal_swd_write_ap(uint8_t reg, uint32_t value);

/**
 * @brief Connect to target (full SWD connect sequence)
 * @return true if connected successfully
 */
bool hal_swd_connect(void);

/**
 * @brief Check if SWD is initialized
 * @return true if initialized
 */
bool hal_swd_is_initialized(void);

#endif /* FXPRO_SWD_H */

/** @} */
