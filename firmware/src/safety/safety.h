/**
 * @file safety.h
 * @brief Safety Monitoring System for FiXPro
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @defgroup safety Safety Monitoring System
 * @{
 *
 * The Safety Monitoring System provides comprehensive protection for both
 * FiXPro and connected target devices. Key features include:
 *
 * - Over-voltage protection with hardware limits
 * - Over-current protection with fast response
 * - Temperature monitoring and thermal shutdown
 * - Real-time voltage and current monitoring
 * - Short circuit detection
 * - Watchdog timer for unresponsive operations
 *
 * Safety Architecture:
 *
 *   +------------------+
 *   |  Safety Monitor  |
 *   |  (Continuous)    |
 *   +--------+---------+
 *            |
 *   +--------+---------+
 *   |        |        |
 *   v        v        v
 * +-----+ +-----+ +------+
 * |Voltage| |Current| |Temp |
 * | ADC  | | ADC  | | ADC |
 * +--+---+ +--+---+ +--+--+
 *    |        |        |
 *    v        v        v
 * Hardware trip circuits
 */

#ifndef OMNIPROG_SAFETY_H
#define OMNIPROG_SAFETY_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * SAFETY CONSTANTS
 *============================================================================*/

/**
 * @name Voltage Limits (millivolts)
 * @brief Safe operating voltage ranges for target devices
 * @{ */
#define SAFETY_VOLTAGE_MIN         1000    /**< Minimum allowed voltage (1.0V) */
#define SAFETY_VOLTAGE_MAX         5500    /**< Maximum allowed voltage (5.5V) */
#define SAFETY_VOLTAGE_DEFAULT     3300    /**< Default target voltage (3.3V) */
#define SAFETY_VOLTAGE_TOLERANCE   100     /**< Voltage tolerance (mV) */
/** @} */

/**
 * @name Current Limits (milliamps)
 * @brief Safe current ranges for target operations
 * @{ */
#define SAFETY_CURRENT_LIMIT       500     /**< Maximum current (500mA) */
#define SAFETY_CURRENT_WARNING     400     /**< Warning threshold (400mA) */
#define SAFETY_CURRENT_SHORT       1000    /**< Short circuit threshold (1A) */
#define SAFETY_CURRENT_DEFAULT     100     /**< Default operating current */
/** @} */

/**
 * @name Temperature Limits (Celsius)
 * @brief Thermal protection thresholds
 * @{ */
#define SAFETY_TEMP_MIN            -20     /**< Minimum operating temp */
#define SAFETY_TEMP_MAX            85      /**< Maximum operating temp */
#define SAFETY_TEMP_WARNING        75      /**< Warning threshold */
#define SAFETY_TEMP_SHUTDOWN       85      /**< Thermal shutdown threshold */
#define SAFETY_TEMP_HYSTERESIS     10      /**< Recovery hysteresis */
/** @} */

/**
 * @name Timing Constants
 * @brief Timing parameters for safety checks
 * @{ */
#define SAFETY_CHECK_INTERVAL_MS   10      /**< Polling interval (10ms) */
#define SAFETY_RESPONSE_TIME_US    100     /**< Max response time (100µs) */
#define SAFETY_WATCHDOG_TIMEOUT_MS  5000    /**< Operation timeout (5s) */
/** @} */

/*============================================================================
 * SAFETY STATUS FLAGS
 *============================================================================*/

/**
 * @brief Safety system status flags
 */
typedef enum {
    SAFETY_STATUS_OK              = 0x00,
    SAFETY_STATUS_INIT            = 0x01,
    SAFETY_STATUS_ACTIVE          = 0x02,
    
    SAFETY_WARN_VOLTAGE_LOW       = 0x10,
    SAFETY_WARN_VOLTAGE_HIGH      = 0x11,
    SAFETY_WARN_CURRENT_HIGH      = 0x12,
    SAFETY_WARN_TEMP_HIGH         = 0x13,
    
    SAFETY_ERROR_VOLTAGE_LOW      = 0x20,
    SAFETY_ERROR_VOLTAGE_HIGH     = 0x21,
    SAFETY_ERROR_OVERCURRENT      = 0x22,
    SAFETY_ERROR_SHORT_CIRCUIT    = 0x23,
    SAFETY_ERROR_OVERTEMPERATURE = 0x24,
    SAFETY_ERROR_TIMEOUT          = 0x25,
    SAFETY_ERROR_NOT_CONNECTED    = 0x26,
    SAFETY_ERROR_VOLTAGE_MISMATCH = 0x27,
    
    SAFETY_FAULT_HARDWARE         = 0x80,
} safety_status_flags_t;

/**
 * @brief Safety status structure
 */
typedef struct {
    uint16_t voltage_mv;         /**< Current voltage in millivolts */
    uint16_t current_ma;         /**< Current draw in milliamps */
    int8_t   temperature_c;       /**< Temperature in Celsius */
    uint8_t  status;             /**< Safety status flags */
    uint16_t voltage_limit;       /**< Configured voltage limit */
    uint16_t current_limit;       /**< Configured current limit */
    uint32_t operation_time_ms;   /**< Time since last operation started */
    uint16_t error_count;         /**< Total safety errors */
    uint16_t warning_count;       /**< Total warnings */
} safety_status_t;

/**
 * @brief Safety configuration structure
 */
typedef struct {
    uint16_t voltage_target;      /**< Target voltage (mV) */
    uint16_t voltage_min;         /**< Minimum voltage (mV) */
    uint16_t voltage_max;         /**< Maximum voltage (mV) */
    uint16_t current_limit;       /**< Current limit (mA) */
    uint8_t  temp_limit;         /**< Temperature limit (°C) */
    bool     auto_protect;       /**< Auto-disconnect on fault */
    bool     warnings_enabled;    /**< Enable warning reporting */
} safety_config_t;

/*============================================================================
 * FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize safety monitoring system
 * @return true if initialization successful
 */
bool safety_init(void);

/**
 * @brief Deinitialize safety system
 */
void safety_deinit(void);

/**
 * @brief Reset safety system to initial state
 */
void safety_reset(void);

/**
 * @brief Get current safety status
 * @return Current safety status structure
 */
safety_status_t safety_get_status(void);

/**
 * @brief Configure safety parameters
 * @param config Pointer to configuration structure
 * @return true if configuration applied successfully
 */
bool safety_configure(const safety_config_t *config);

/**
 * @brief Get current configuration
 * @param config Pointer to store current configuration
 */
void safety_get_config(safety_config_t *config);

/**
 * @brief Check if target voltage is safe
 * @param voltage_mv Voltage to check in millivolts
 * @return true if voltage is within safe range
 */
bool safety_check_voltage(uint16_t voltage_mv);

/**
 * @brief Check if current is within safe limits
 * @param current_ma Current to check in milliamps
 * @return true if current is safe
 */
bool safety_check_current(uint16_t current_ma);

/**
 * @brief Check if temperature is within safe range
 * @param temp_c Temperature to check in Celsius
 * @return true if temperature is safe
 */
bool safety_check_temperature(int8_t temp_c);

/**
 * @brief Perform comprehensive safety check
 * @return Combined safety status
 */
safety_status_flags_t safety_check_all(void);

/**
 * @brief Enable target power
 * @return true if power enabled successfully
 */
bool safety_enable_power(void);

/**
 * @brief Disable target power (emergency stop)
 */
void safety_disable_power(void);

/**
 * @brief Check if power is currently enabled
 * @return true if power is on
 */
bool safety_is_power_enabled(void);

/**
 * @brief Start an operation (reset watchdog)
 */
void safety_operation_start(void);

/**
 * @brief End current operation
 */
void safety_operation_end(void);

/**
 * @brief Check if operation timed out
 * @return true if operation exceeded timeout
 */
bool safety_operation_timeout(void);

/**
 * @brief Get last safety error
 * @return Last error code
 */
safety_status_flags_t safety_get_last_error(void);

/**
 * @brief Clear error count
 */
void safety_clear_errors(void);

/**
 * @brief Get human-readable status string
 * @param status Status code
 * @return Status description string
 */
const char* safety_status_string(safety_status_flags_t status);

/**
 * @brief Periodic safety check (call in main loop)
 */
void safety_poll(void);

/*============================================================================
 * CALLBACKS (implement in application)
 *============================================================================*/

/**
 * @brief Called when safety warning occurs
 * @param warning Warning code
 */
void safety_on_warning(safety_status_flags_t warning);

/**
 * @brief Called when safety error occurs
 * @param error Error code
 */
void safety_on_error(safety_status_flags_t error);

/**
 * @brief Called when power is disabled due to safety
 */
void safety_on_power_disabled(void);

#endif /* OMNIPROG_SAFETY_H */

/** @} */ /* end of safety group */
