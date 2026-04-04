/**
 * @file main.c
 * @brief FiXPro - Using pico_stdio_usb with descriptors
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "tusb.h"

#define LED_PIN 25

// USB Vendor/Product IDs
#define USBD_VID           0x2E8A
#define USBD_PID           0x000A
#define USBD_MANUFACTURER  "FiXPro"
#define USBD_PRODUCT       "FiXPro Universal Programmer"

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

// Configuration descriptor
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

uint8_t const desc_cfg[CONFIG_TOTAL_LEN] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_CDC_DESCRIPTOR(0, 0, 0x81, 8, 0x02, 0x82, 64),
};

// String descriptors
static char serial_str[32];

char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    USBD_MANUFACTURER,
    USBD_PRODUCT,
    serial_str,
};

// TinyUSB callbacks
uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*) &desc_device;
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_cfg;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    static uint16_t desc_str[32];
    
    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        desc_str[0] = (TUSB_DESC_STRING << 8) | (2 + 2);
        return desc_str;
    }
    
    if (index >= 4) return NULL;
    
    const char* str = string_desc_arr[index];
    uint8_t len = strlen(str);
    
    for (uint8_t i = 0; i < len && i < 31; i++) {
        desc_str[1 + i] = str[i];
    }
    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
    return desc_str;
}

int main() {
    // Get unique serial
    pico_get_unique_board_id_string(serial_str, sizeof(serial_str));
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Blink 3 times - start
    for(int i=0; i<3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    stdio_init_all();
    
    // Blink 3 times - after init
    for(int i=0; i<3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    printf("FiXPro USB Test v2.0.0\n");
    
    while (true) {
        tud_task();
        gpio_put(LED_PIN, tud_cdc_connected() ? 1 : 0);
    }
}
