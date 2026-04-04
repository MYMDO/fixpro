cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

project(FiXPro C CXX ASM)

set(PICO_BOARD pico CACHE STRING "Pico board variant")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH not set. Set it with -DPICO_SDK_PATH=/path/to/sdk")
endif()

message(STATUS "Pico SDK path: ${PICO_SDK_PATH}")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

pico_sdk_init()

set(FIRMWARE_NAME FiXPro)
set(FIRMWARE_VERSION 2.0.0)

add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -O2
)

add_definitions(
    -D_FIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/pio
    ${PICO_SDK_PATH}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(FIRMWARE_SOURCES
    src/main.c
)

add_executable(${FIRMWARE_NAME}
    ${FIRMWARE_SOURCES}
)

target_compile_definitions(${FIRMWARE_NAME} PRIVATE
    CFG_TUD_ENABLED=1
    CFG_TUD_CDC=1
)

target_link_libraries(${FIRMWARE_NAME}
    pico_stdlib
    hardware_gpio
    tinyusb_device
    tinyusb_board
)

pico_set_program_name(${FIRMWARE_NAME} "FiXPro")
pico_set_program_version(${FIRMWARE_NAME} "${FIRMWARE_VERSION}")

pico_add_extra_outputs(${FIRMWARE_NAME})

pico_enable_stdio_uart(${FIRMWARE_NAME} 0)
pico_enable_stdio_usb(${FIRMWARE_NAME} 1)
