#pragma once

#include <input.h>
#include <graphics.h>

typedef struct {
	uint16_t * text;
	size_2d_t size; // size in characters
	// cursor posiion
	int x;
	int y;
	uint32_t colour; // current colour
	rect_2d_t rect; // output image
	int mode; // output mode
} text_buffer_t;

enum {
	TEXT_BUFFER_ALPHA, // enable 32 bit colour
	TEXT_BUFFER_FALPHA, // 24 bit colour with fast transparency (rect_2d_afdraw)
	TEXT_BUFFER_24BIT, // no transparency
};

void text_buffer_write(text_buffer_t * buffer, uint16_t * data);
void text_buffer_writec(text_buffer_t * buffer, uint16_t c);

