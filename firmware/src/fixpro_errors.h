/**
 * @file fixpro_errors.h
 * @brief FiXPro Error Codes and Status Definitions
 * @author FiXPro Contributors
 * @version 2.0.0
 * @license GPL-3.0
 */

#ifndef FXPRO_ERRORS_H
#define FXPRO_ERRORS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * STATUS CODES
 *============================================================================*/

/** @name Success Status
 * @{ */
#define FXPRO_OK              0x00
#define FXPRO_OK_PENDING      0x01
/** @} */

/** @name General Errors
 * @{ */
#define FXPRO_ERROR           0x10
#define FXPRO_ERROR_PARAM     0x11
#define FXPRO_ERROR_TIMEOUT   0x12
#define FXPRO_ERROR_BUSY      0x13
#define FXPRO_ERROR_NO_DEVICE 0x14
#define FXPRO_ERROR_INIT      0x15
/** @} */

/** @name Hardware Errors
 * @{ */
#define FXPRO_ERROR_BUS      0x20
#define FXPRO_ERROR_NACK     0x21
#define FXPRO_ERROR_NO_RESPONSE 0x22
#define FXPRO_ERROR_VERIFY    0x23
#define FXPRO_ERROR_ERASE    0x24
#define FXPRO_ERROR_WRITE     0x25
#define FXPRO_ERROR_READ      0x26
/** @} */

/** @name Safety Errors
 * @{ */
#define FXPRO_ERROR_SAFETY   0x30
#define FXPRO_ERROR_OVP      0x31
#define FXPRO_ERROR_OCP      0x32
#define FXPRO_ERROR_OTP       0x33
#define FXPRO_ERROR_UVP      0x34
/** @} */

/** @name Debug Interface Errors
 * @{ */
#define FXPRO_ERROR_JTAG_INIT    0x40
#define FXPRO_ERROR_JTAG_TIMEOUT 0x41
#define FXPRO_ERROR_SWD_INIT     0x42
#define FXPRO_ERROR_SWD_FAULT    0x43
#define FXPRO_ERROR_DAP_ACCESS   0x44
/** @} */

/*============================================================================
 * STATUS TO STRING
 *============================================================================*/

/**
 * @brief Convert status code to string
 * @param status Status code
 * @return Human-readable status string
 */
static inline const char* fixpro_status_str(uint8_t status)
{
    switch (status) {
        case FXPRO_OK:              return "OK";
        case FXPRO_OK_PENDING:      return "Pending";
        case FXPRO_ERROR:           return "Error";
        case FXPRO_ERROR_PARAM:     return "Invalid parameter";
        case FXPRO_ERROR_TIMEOUT:   return "Timeout";
        case FXPRO_ERROR_BUSY:      return "Device busy";
        case FXPRO_ERROR_NO_DEVICE: return "No device";
        case FXPRO_ERROR_INIT:      return "Initialization failed";
        case FXPRO_ERROR_BUS:       return "Bus error";
        case FXPRO_ERROR_NACK:      return "No ACK";
        case FXPRO_ERROR_NO_RESPONSE: return "No response";
        case FXPRO_ERROR_VERIFY:    return "Verification failed";
        case FXPRO_ERROR_ERASE:     return "Erase failed";
        case FXPRO_ERROR_WRITE:     return "Write failed";
        case FXPRO_ERROR_READ:      return "Read failed";
        case FXPRO_ERROR_SAFETY:    return "Safety violation";
        case FXPRO_ERROR_OVP:       return "Over-voltage";
        case FXPRO_ERROR_OCP:       return "Over-current";
        case FXPRO_ERROR_OTP:       return "Over-temperature";
        case FXPRO_ERROR_UVP:       return "Under-voltage";
        case FXPRO_ERROR_JTAG_INIT:    return "JTAG init failed";
        case FXPRO_ERROR_JTAG_TIMEOUT: return "JTAG timeout";
        case FXPRO_ERROR_SWD_INIT:     return "SWD init failed";
        case FXPRO_ERROR_SWD_FAULT:    return "SWD fault";
        case FXPRO_ERROR_DAP_ACCESS:   return "DAP access failed";
        default:                    return "Unknown";
    }
}

/**
 * @brief Check if status indicates success
 * @param status Status code
 * @return true if success
 */
static inline bool fixpro_success(uint8_t status)
{
    return status < 0x10;
}

/**
 * @brief Check if status indicates error
 * @param status Status code
 * @return true if error
 */
static inline bool fixpro_error(uint8_t status)
{
    return status >= 0x10;
}

#ifdef __cplusplus
}
#endif

#endif /* FXPRO_ERRORS_H */
