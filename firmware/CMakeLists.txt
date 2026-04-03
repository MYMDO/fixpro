cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

project(FiXPro C CXX ASM)

set(PICO_BOARD pico CACHE STRING "Pico board variant")

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH not set. Set it with -DPICO_SDK_PATH=/path/to/sdk")
endif()

message(STATUS "Pico SDK path: ${PICO_SDK_PATH}")
include(${PICO_SDK_PATH}/cmake/pico_sdk_import.cmake)

pico_sdk_init()

set(FIRMWARE_NAME FiXPro)
set(FIRMWARE_VERSION 1.0.0)

add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -O2
    -flto
)

add_definitions(-D_FIRMWARE_VERSION=\"${FIRMWARE_VERSION}\")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/pio
    ${PICO_SDK_PATH}/src
)

set(FIRMWARE_SOURCES
    src/main.c
    src/hal/hal.c
    src/safety/safety.c
    src/usb/usb_protocol.c
    src/flash/spi_flash.c
)

set(FIRMWARE_HEADERS
    src/main.h
    src/hal/hal.h
    src/safety/safety.h
    src/usb/usb_protocol.h
    src/flash/spi_flash.h
)

set(PIO_SOURCES
    pio/spi.pio
    pio/i2c.pio
)

pico_generate_pio_header(${FIRMWARE_NAME} ${PIO_SOURCES})

add_executable(${FIRMWARE_NAME}
    ${FIRMWARE_SOURCES}
    ${FIRMWARE_HEADERS}
    ${PIO_SOURCES}
)

target_link_libraries(${FIRMWARE_NAME}
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_i2c
    hardware_adc
    hardware_pwm
    hardware_clocks
    hardware_pll
    hardware_irq
    pico_multicore
    tinyusb_device
    tinyusb_board
)

pico_set_program_name(${FIRMWARE_NAME} "FiXPro")
pico_set_program_version(${FIRMWARE_NAME} "${FIRMWARE_VERSION}")
pico_set_bootrom_hint(${FIRMWARE_NAME} "none")

pico_add_extra_outputs(${FIRMWARE_NAME})

pico_enable_stdio_uart(${FIRMWARE_NAME} 0)
pico_enable_stdio_usb(${FIRMWARE_NAME} 1)

add_custom_command(TARGET ${FIRMWARE_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${FIRMWARE_NAME}.elf ${FIRMWARE_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${FIRMWARE_NAME}.elf ${FIRMWARE_NAME}.hex
    COMMAND echo "Firmware built: ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.uf2"
)

print_target_config(${FIRMWARE_NAME})
