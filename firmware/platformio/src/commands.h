/**
 * @file commands.h
 * @brief FiXPro Command Handler
 * @version 2.1.0
 */
#ifndef FIXPRO_COMMANDS_H
#define FIXPRO_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>

#define CMD_BUFFER_SIZE 256

#define CMD_TIMEOUT_MS 5000

typedef void (*CommandHandler)(void);

typedef struct {
    const char* name;
    CommandHandler handler;
} CommandEntry;

void cmd_ping(void);
void cmd_caps(void);
void cmd_version(void);
void cmd_gpio(void);
void cmd_gpio_set(void);
void cmd_gpio_clr(void);
void cmd_spi_id(void);
void cmd_spi_read(uint32_t addr, uint32_t len);
void cmd_spi_write(uint32_t addr, const uint8_t* data, uint32_t len);
void cmd_spi_erase(uint32_t addr, uint32_t len);
void cmd_spi_erase_chip(void);
void cmd_i2c_scan(void);
void cmd_i2c_read(uint8_t addr, uint8_t reg, uint8_t len);
void cmd_i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
void cmd_status(void);
void cmd_help(void);
void cmd_reset(void);
void cmd_info(void);

void process_command(const char* cmd);
void process_char(char c);
void init_commands(void);

#endif
