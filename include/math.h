#pragma once

#include <stdint.h>

typedef struct vect_2d {
	int x;
	int y;
} vect_2d_t;

uint32_t round32(uint32_t x, uint32_t y);
int scale_range32(int value, int xMin, int xMax, int yMin, int yMax);
long double pow(long double, long double);
int abs32(int x);
signed short abs16(signed short x);
signed char abs8(signed char x);
uint16_t * number_text_suffix(int i);