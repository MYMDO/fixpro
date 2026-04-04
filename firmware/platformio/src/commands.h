/**
 * @file commands.h
 * @brief FiXPro Command Handler
 */
#ifndef FIXPRO_COMMANDS_H
#define FIXPRO_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>

#define CMD_BUFFER_SIZE 256

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
void cmd_i2c_scan(void);
void cmd_status(void);
void cmd_help(void);
void cmd_reset(void);
void cmd_info(void);

void process_command(const char* cmd);
void process_char(char c);
void init_commands(void);

#endif
