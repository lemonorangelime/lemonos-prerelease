#pragma once

#include <stdint.h>

typedef struct memory_block {
	uint32_t state;
	uint32_t size;
} __attribute__((packed)) memory_block_t;

// states a block can be in
enum {
	MEMORY_NON_EXISTENT,
	MEMORY_FREE,
	MEMORY_IN_USE,
};

extern void * _kernel_end;
extern void * _kernel_start;
extern void * _kernel_size;

extern void * heap;
extern void * heap_end;
extern size_t ram_size;
extern size_t heap_length;
extern size_t used_memory;

void mmap_parse();
void * calloc(size_t number, size_t size);
void * realloc(void * p, size_t size);
void * malloc(size_t size);
int free(void * data);

void memory_init();
void memory_late_init();