/**
 * @file jtag.h
 * @brief JTAG Hardware Abstraction Layer
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @defgroup jtag JTAG Interface
 * @{
 */

#ifndef FXPRO_JTAG_H
#define FXPRO_JTAG_H

#include <stdint.h>
#include <stdbool.h>

#define JTAG_TCK_PIN    6
#define JTAG_TMS_PIN    7
#define JTAG_TDI_PIN    8
#define JTAG_TDO_PIN    9

#define JTAG_TCK_SM     0
#define JTAG_TMS_SM     1

/**
 * @brief Initialize JTAG interface using PIO
 * @return true if initialization successful
 */
bool hal_jtag_init(void);

/**
 * @brief Deinitialize JTAG interface
 */
void hal_jtag_deinit(void);

/**
 * @brief Perform JTAG reset (TMS high for 5+ cycles)
 */
void hal_jtag_reset(void);

/**
 * @brief Shift data through JTAG chain
 * @param tdi_data Data to shift out on TDI
 * @param num_bits Number of bits to shift
 * @param tdo_data Buffer to store TDO data
 * @return true if successful
 */
bool hal_jtag_shift(const uint8_t *tdi_data, uint16_t num_bits, uint8_t *tdo_data);

/**
 * @brief Read IDCODE from JTAG chain
 * @return 32-bit IDCODE or 0 if no device
 */
uint32_t hal_jtag_read_idcode(void);

/**
 * @brief Check if JTAG is initialized
 * @return true if initialized
 */
bool hal_jtag_is_initialized(void);

#endif /* FXPRO_JTAG_H */

/** @} */
