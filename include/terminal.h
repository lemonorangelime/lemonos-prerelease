#pragma once

#include <stdint.h>

void terminal_print8l(char * string, size_t len);
void terminal_cprint(uint16_t * string, uint32_t colour);
void terminal_cputc(uint16_t character, uint32_t colour);
void terminal_print(uint16_t * string);
void terminal_putc(uint16_t character);
void terminal_init();
