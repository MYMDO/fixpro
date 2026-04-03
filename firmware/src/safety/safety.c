/**
 * @file safety.c
 * @brief Safety Monitoring System Implementation
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 */

#include "safety.h"
#include "../hal/hal.h"
#include "pico/time.h"
#include <string.h>

/*============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/

static bool safety_initialized = false;
static bool power_enabled = false;
static bool operation_active = false;

static safety_config_t current_config = {
    .voltage_target = SAFETY_VOLTAGE_DEFAULT,
    .voltage_min = SAFETY_VOLTAGE_MIN,
    .voltage_max = SAFETY_VOLTAGE_MAX,
    .current_limit = SAFETY_CURRENT_LIMIT,
    .temp_limit = SAFETY_TEMP_MAX,
    .auto_protect = true,
    .warnings_enabled = true,
};

static safety_status_t current_status = {
    .voltage_mv = 0,
    .current_ma = 0,
    .temperature_c = 25,
    .status = SAFETY_STATUS_INIT,
    .voltage_limit = SAFETY_VOLTAGE_MAX,
    .current_limit = SAFETY_CURRENT_LIMIT,
    .operation_time_ms = 0,
    .error_count = 0,
    .warning_count = 0,
};

static safety_status_flags_t last_error = SAFETY_STATUS_OK;

static absolute_time_t operation_start_time;
static absolute_time_t last_check_time;

static uint32_t poll_interval_ms = SAFETY_CHECK_INTERVAL_MS;

static volatile bool fault_condition = false;

/*============================================================================
 * POWER CONTROL HARDWARE
 *============================================================================*/

/**
 * @brief Initialize power control hardware
 */
static void power_hw_init(void)
{
    gpio_init(20);
    gpio_set_dir(20, GPIO_OUT);
    gpio_put(20, false);
}

/**
 * @brief Enable power to target
 */
static void power_hw_enable(void)
{
    gpio_put(20, true);
}

/**
 * @brief Disable power to target
 */
static void power_hw_disable(void)
{
    gpio_put(20, false);
}

/*============================================================================
 * MEASUREMENT FUNCTIONS
 *============================================================================*/

/**
 * @brief Read current voltage from ADC
 */
static uint16_t read_voltage(void)
{
    uint16_t raw = hal_adc_read_voltage_mv(HAL_ADC_VSENSE_PIN);
    
    return raw * 11;
}

/**
 * @brief Read current current from ADC
 */
static uint16_t read_current(void)
{
    uint16_t sense_resistance = 100;
    return hal_adc_read_current_ma(sense_resistance);
}

/**
 * @brief Read temperature from internal sensor
 */
static int8_t read_temperature(void)
{
    return hal_adc_read_temperature();
}

/*============================================================================
 * SAFETY CHECK FUNCTIONS
 *============================================================================*/

/**
 * @brief Check voltage against limits
 */
static safety_status_flags_t check_voltage(uint16_t voltage_mv)
{
    if (voltage_mv < current_config.voltage_min) {
        if (voltage_mv < 500) {
            return SAFETY_ERROR_NOT_CONNECTED;
        }
        return SAFETY_ERROR_VOLTAGE_LOW;
    }
    
    if (voltage_mv > current_config.voltage_max) {
        return SAFETY_ERROR_VOLTAGE_HIGH;
    }
    
    int16_t diff = voltage_mv - current_config.voltage_target;
    if (diff < 0) diff = -diff;
    
    if (diff > SAFETY_VOLTAGE_TOLERANCE * 2) {
        return SAFETY_WARN_VOLTAGE_LOW;
    }
    
    return SAFETY_STATUS_OK;
}

/**
 * @brief Check current against limits
 */
static safety_status_flags_t check_current(uint16_t current_ma)
{
    if (current_ma > SAFETY_CURRENT_SHORT) {
        return SAFETY_ERROR_SHORT_CIRCUIT;
    }
    
    if (current_ma > current_config.current_limit) {
        return SAFETY_ERROR_OVERCURRENT;
    }
    
    if (current_ma > SAFETY_CURRENT_WARNING) {
        return SAFETY_WARN_CURRENT_HIGH;
    }
    
    return SAFETY_STATUS_OK;
}

/**
 * @brief Check temperature against limits
 */
static safety_status_flags_t check_temperature(int8_t temp_c)
{
    if (temp_c > SAFETY_TEMP_SHUTDOWN) {
        return SAFETY_ERROR_OVERTEMPERATURE;
    }
    
    if (temp_c > SAFETY_TEMP_WARNING) {
        return SAFETY_WARN_TEMP_HIGH;
    }
    
    return SAFETY_STATUS_OK;
}

/*============================================================================
 * PUBLIC API IMPLEMENTATION
 *============================================================================*/

bool safety_init(void)
{
    if (safety_initialized) {
        return true;
    }
    
    hal_adc_init();
    power_hw_init();
    
    current_status.status = SAFETY_STATUS_INIT;
    last_error = SAFETY_STATUS_OK;
    fault_condition = false;
    
    safety_initialized = true;
    
    return true;
}

void safety_deinit(void)
{
    if (!safety_initialized) {
        return;
    }
    
    safety_disable_power();
    safety_initialized = false;
}

void safety_reset(void)
{
    memset(&current_status, 0, sizeof(current_status));
    current_status.status = SAFETY_STATUS_INIT;
    current_status.voltage_limit = current_config.voltage_max;
    current_status.current_limit = current_config.current_limit;
    last_error = SAFETY_STATUS_OK;
    fault_condition = false;
    power_enabled = false;
    operation_active = false;
}

safety_status_t safety_get_status(void)
{
    current_status.voltage_mv = read_voltage();
    current_status.current_ma = read_current();
    current_status.temperature_c = read_temperature();
    current_status.status = safety_check_all();
    
    if (operation_active) {
        current_status.operation_time_ms = 
            (uint32_t)(absolute_time_diff_us(operation_start_time, get_absolute_time()) / 1000);
    } else {
        current_status.operation_time_ms = 0;
    }
    
    return current_status;
}

bool safety_configure(const safety_config_t *config)
{
    if (config == NULL) {
        return false;
    }
    
    if (config->voltage_target < SAFETY_VOLTAGE_MIN || 
        config->voltage_target > SAFETY_VOLTAGE_MAX) {
        return false;
    }
    
    if (config->voltage_min >= config->voltage_max) {
        return false;
    }
    
    if (config->current_limit > SAFETY_CURRENT_SHORT) {
        return false;
    }
    
    memcpy(&current_config, config, sizeof(safety_config_t));
    
    current_status.voltage_limit = config->voltage_max;
    current_status.current_limit = config->current_limit;
    
    return true;
}

void safety_get_config(safety_config_t *config)
{
    if (config != NULL) {
        memcpy(config, &current_config, sizeof(safety_config_t));
    }
}

bool safety_check_voltage(uint16_t voltage_mv)
{
    return check_voltage(voltage_mv) == SAFETY_STATUS_OK;
}

bool safety_check_current(uint16_t current_ma)
{
    return check_current(current_ma) == SAFETY_STATUS_OK;
}

bool safety_check_temperature(int8_t temp_c)
{
    return check_temperature(temp_c) == SAFETY_STATUS_OK;
}

safety_status_flags_t safety_check_all(void)
{
    uint16_t voltage = read_voltage();
    uint16_t current = read_current();
    int8_t temperature = read_temperature();
    
    safety_status_flags_t status = SAFETY_STATUS_OK;
    
    safety_status_flags_t v_status = check_voltage(voltage);
    if (v_status != SAFETY_STATUS_OK) {
        status = v_status;
    }
    
    safety_status_flags_t c_status = check_current(current);
    if (c_status != SAFETY_STATUS_OK) {
        status = c_status;
    }
    
    safety_status_flags_t t_status = check_temperature(temperature);
    if (t_status != SAFETY_STATUS_OK) {
        status = t_status;
    }
    
    if (operation_active && safety_operation_timeout()) {
        status = SAFETY_ERROR_TIMEOUT;
    }
    
    return status;
}

bool safety_enable_power(void)
{
    if (!safety_initialized) {
        return false;
    }
    
    if (fault_condition) {
        return false;
    }
    
    uint16_t voltage = read_voltage();
    
    if (voltage > 500) {
        last_error = SAFETY_ERROR_VOLTAGE_MISMATCH;
        current_status.error_count++;
        return false;
    }
    
    power_hw_enable();
    power_enabled = true;
    current_status.status = SAFETY_STATUS_ACTIVE;
    
    return true;
}

void safety_disable_power(void)
{
    power_hw_disable();
    power_enabled = false;
    
    if (safety_is_power_enabled() == false) {
        safety_on_power_disabled();
    }
}

bool safety_is_power_enabled(void)
{
    return power_enabled;
}

void safety_operation_start(void)
{
    operation_start_time = get_absolute_time();
    operation_active = true;
}

void safety_operation_end(void)
{
    operation_active = false;
}

bool safety_operation_timeout(void)
{
    if (!operation_active) {
        return false;
    }
    
    uint32_t elapsed = (uint32_t)(absolute_time_diff_us(operation_start_time, get_absolute_time()) / 1000);
    return elapsed > SAFETY_WATCHDOG_TIMEOUT_MS;
}

safety_status_flags_t safety_get_last_error(void)
{
    return last_error;
}

void safety_clear_errors(void)
{
    current_status.error_count = 0;
    current_status.warning_count = 0;
    last_error = SAFETY_STATUS_OK;
    fault_condition = false;
}

const char* safety_status_string(safety_status_flags_t status)
{
    switch (status) {
        case SAFETY_STATUS_OK:              return "OK";
        case SAFETY_STATUS_INIT:            return "Initializing";
        case SAFETY_STATUS_ACTIVE:          return "Active";
        case SAFETY_WARN_VOLTAGE_LOW:       return "Warning: Voltage low";
        case SAFETY_WARN_VOLTAGE_HIGH:      return "Warning: Voltage high";
        case SAFETY_WARN_CURRENT_HIGH:      return "Warning: Current high";
        case SAFETY_WARN_TEMP_HIGH:         return "Warning: Temperature high";
        case SAFETY_ERROR_VOLTAGE_LOW:      return "Error: Voltage too low";
        case SAFETY_ERROR_VOLTAGE_HIGH:     return "Error: Voltage too high";
        case SAFETY_ERROR_OVERCURRENT:      return "Error: Overcurrent";
        case SAFETY_ERROR_SHORT_CIRCUIT:    return "Error: Short circuit";
        case SAFETY_ERROR_OVERTEMPERATURE:  return "Error: Overtemperature";
        case SAFETY_ERROR_TIMEOUT:          return "Error: Operation timeout";
        case SAFETY_ERROR_NOT_CONNECTED:    return "Error: Target not connected";
        case SAFETY_ERROR_VOLTAGE_MISMATCH: return "Error: Voltage mismatch";
        case SAFETY_FAULT_HARDWARE:         return "Hardware fault";
        default:                            return "Unknown status";
    }
}

void safety_poll(void)
{
    if (!safety_initialized) {
        return;
    }
    
    safety_status_flags_t status = safety_check_all();
    
    if (status != SAFETY_STATUS_OK && status != SAFETY_STATUS_ACTIVE) {
        if (status >= 0x20) {
            last_error = status;
            current_status.error_count++;
            safety_on_error(status);
            
            if (current_config.auto_protect) {
                safety_disable_power();
                fault_condition = true;
            }
        } else if (current_config.warnings_enabled) {
            current_status.warning_count++;
            safety_on_warning(status);
        }
    }
    
    last_check_time = get_absolute_time();
}

/*============================================================================
 * CALLBACK WEAK IMPLEMENTATIONS
 *============================================================================*/

__attribute__((weak)) void safety_on_warning(safety_status_flags_t warning)
{
    (void)warning;
}

__attribute__((weak)) void safety_on_error(safety_status_flags_t error)
{
    (void)error;
}

__attribute__((weak)) void safety_on_power_disabled(void)
{
}
