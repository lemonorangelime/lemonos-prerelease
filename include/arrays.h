#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
	void * input;
	void * output;
	size_t size;
	size_t step;
} bytes_iterator_t;

void bytes_shift(bytes_iterator_t * iterator, int step);
void * bytes_step_iterator(bytes_iterator_t * iterator);