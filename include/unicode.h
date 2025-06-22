#pragma once

#include <stdint.h>

uint8_t first_zero(uint8_t byte);
int utf8_strlen(char * string);
void utf8toutf16(char * in, uint16_t * out);
void utf8toutf16l(char * in, uint16_t * out, int size);