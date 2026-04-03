/**
 * @file main.c
 * @brief FiXPro Flash iX Pro - Main Entry Point
 * @author FiXPro Contributors
 * @version 1.0.0
 * @license GPL-3.0
 *
 * @mainpage FiXPro
 *
 * Universal Hardware Programmer Engine built on Raspberry Pi RP2040
 *
 * Features:
 * - High-speed SPI Flash programming (up to 50MHz)
 * - I2C EEPROM support (up to 1MHz)
 * - Comprehensive safety monitoring
 * - USB CDC communication
 *
 * @see https://fx.in.ua
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/unique_id.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"

#include "usb/usb_protocol.h"
#include "safety/safety.h"
#include "hal/hal.h"

#include "flash/spi_flash.h"

/*============================================================================
 * CONFIGURATION
 *============================================================================*/

/**
 * @brief System clock configuration
 */
#ifndef SYS_CLK_KHZ
#define SYS_CLK_KHZ 125000
#endif

/**
 * @brief USB task priority
 */
#define USB_TASK_PRIORITY 8

/**
 * @brief Core1 stack size
 */
#define CORE1_STACK_SIZE 2048

/*============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/

/**
 * @brief Device serial number (from OTP)
 */
static char device_serial[13];

/**
 * @brief Firmware version string
 */
static const char firmware_version[] = "1.0.0";

/**
 * @brief Core1 stack
 */
static uint8_t core1_stack[CORE1_STACK_SIZE];

/**
 * @brief Flag indicating device is ready
 */
static volatile bool device_ready = false;

/**
 * @brief Blinking state for LED
 */
static volatile uint32_t led_blink_count = 0;

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

static void core1_entry(void);
static void system_clock_init(void);
static void led_set_state(bool on);
static void led_blink(void);

/*============================================================================
 * CALLBACK IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Safety warning callback
 */
void safety_on_warning(safety_status_flags_t warning)
{
    printf("[SAFETY] Warning: %s\n", safety_status_string(warning));
}

/**
 * @brief Safety error callback
 */
void safety_on_error(safety_status_flags_t error)
{
    printf("[SAFETY] Error: %s\n", safety_status_string(error));
    led_set_state(false);
}

/**
 * @brief Safety power disabled callback
 */
void safety_on_power_disabled(void)
{
    printf("[SAFETY] Power disabled due to safety condition\n");
    led_set_state(false);
}

/*============================================================================
 * SYSTEM INITIALIZATION
 *============================================================================*/

/**
 * @brief Initialize system clock
 */
static void system_clock_init(void)
{
    set_sys_clock_khz(SYS_CLK_KHZ, true);
    
    uint f_pll_sys = frequency_count_khz(PLL_SYS_FREQ_KHZ);
    uint f_pll_usb = frequency_count_khz(PLL_USB_FREQ_KHZ);
    uint f_clk_sys = frequency_count_khz(CLK_SYS);
    
    printf("[SYS] System clock: %u kHz\n", f_clk_sys);
    printf("[SYS] PLL SYS: %u kHz\n", f_pll_sys);
    printf("[SYS] PLL USB: %u kHz\n", f_pll_usb);
}

/**
 * @brief Initialize LED
 */
static void led_init(void)
{
    hal_led_init();
    hal_led_set_brightness(0);
}

/**
 * @brief Set LED state
 */
static void led_set_state(bool on)
{
    if (on) {
        hal_led_set_brightness(255);
    } else {
        hal_led_set_brightness(0);
    }
}

/**
 * @brief Blink LED once
 */
static void led_blink(void)
{
    led_set_state(true);
    sleep_ms(50);
    led_set_state(false);
}

/**
 * @brief Read device serial number
 */
static void read_device_serial(void)
{
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    
    snprintf(device_serial, sizeof(device_serial), 
             "%02X%02X%02X%02X",
             board_id.id[0], board_id.id[1],
             board_id.id[2], board_id.id[3]);
    
    printf("[SYS] Device serial: %s\n", device_serial);
}

/*============================================================================
 * CORE1 HANDLING
 *============================================================================*/

/**
 * @brief Core1 entry point
 * Handles background tasks
 */
static void core1_entry(void)
{
    printf("[CORE1] Started on core 1\n");
    
    while (true) {
        safety_poll();
        
        sleep_ms(10);
    }
}

/*============================================================================
 * MAIN ENTRY POINT
 *============================================================================*/

/**
 * @brief Main entry point
 */
int main(void)
{
    stdio_init_all();
    
    printf("\n");
    printf("=================================================\n");
    printf("  FiXPro - Flash iX Pro\n");
    printf("  Version: %s\n", firmware_version);
    printf("  Build: %s %s\n", __DATE__, __TIME__);
    printf("=================================================\n");
    printf("\n");
    
    system_clock_init();
    read_device_serial();
    
    printf("[INIT] Initializing hardware...\n");
    hal_gpio_init();
    led_init();
    
    printf("[INIT] Initializing safety system...\n");
    safety_init();
    
    printf("[INIT] Initializing USB...\n");
    usb_protocol_init();
    
    printf("[INIT] Initializing SPI Flash driver...\n");
    spi_flash_init();
    
    printf("[INIT] Starting core1 background task...\n");
    multicore_launch_core1_with_stack(core1_entry, core1_stack, sizeof(core1_stack));
    
    printf("[INIT] System ready!\n");
    printf("\n");
    printf("FiXPro ready. Connect via USB CDC to begin.\n");
    printf("\n");
    
    device_ready = true;
    led_set_state(true);
    
    while (true) {
        usb_packet_t packet;
        
        safety_poll();
        
        if (usb_receive_packet(&packet, 100)) {
            led_blink();
            usb_protocol_dispatch(&packet);
        }
        
        if (led_blink_count > 0) {
            led_blink_count--;
            led_blink();
        }
    }
    
    return 0;
}
