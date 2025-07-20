#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct allocator allocator_t;

typedef void * (* malloc_t)(allocator_t * allocator, size_t size);
typedef int (* free_t)(allocator_t * allocator, void * p);

typedef struct allocator {
	uint16_t * name;
	void * base;
	uint32_t base_addr;
	void * end;
	uint32_t size;
	malloc_t malloc;
	free_t free;
} allocator_t;

void * allocators_malloc(allocator_t * allocator, size_t size);
int allocators_free(allocator_t * allocator, void * p);
allocator_t * allocators_create(uint16_t * name, void * base, uint32_t size);