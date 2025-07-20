#include <memory/allocators.h>
#include <interrupts/irq.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <util.h>

typedef struct {
	uint32_t state;
	uint32_t size;
} __attribute__((packed)) generic_block_t;

enum {
	MEMORY_GENERIC_NON_EXISTENT,
	MEMORY_GENERIC_FREE,
	MEMORY_GENERIC_IN_USE,
};

void * generic_malloc(allocator_t * allocator, size_t size) {
	size_t required_size = round32(size, 16);
	generic_block_t * current_block = (generic_block_t *) allocator->base;
	void * current = allocator->base;
	int save = interrupts_enabled();
	if ((required_size % 8) == 0) {
		required_size += 8;
	}
	if ((size == 0) || ((allocator->base + required_size + sizeof(generic_block_t)) > allocator->end)) {
		return NULL;
	}
	disable_interrupts();
	while ((current_block->state == MEMORY_FREE) || (current_block->state == MEMORY_IN_USE)) {
		if (current_block->state == MEMORY_FREE) {
			if (current_block->size >= required_size) {
				// split(current_block, required_size);
				current_block->state = MEMORY_IN_USE;
				interrupts_restore(save);
				return (current - allocator->base_addr) + sizeof(generic_block_t);
			}
			generic_block_t * next = current + current_block->size + sizeof(generic_block_t);
			if (next->state == MEMORY_NON_EXISTENT) {
				break;
			}
		}
		current += current_block->size + sizeof(generic_block_t);
		current_block = (generic_block_t *) current;
		if (current > allocator->end || current_block->state > 3) {
			//printf(u"Allocator error, %s.\n", (current_block->state > 3) ? u"Memory Corruption" : u"Out of memory");
			interrupts_restore(save);
			return NULL;
		}
	}
	memset32(current + current_block->size + sizeof(generic_block_t), 0, 4);

	// create new block
	current_block->state = MEMORY_IN_USE;
	current_block->size = required_size;
	interrupts_restore(save);
	return (current - allocator->base_addr) + sizeof(generic_block_t);
}

int generic_free(allocator_t * allocator, void * p) {
	generic_block_t * block;
	int save = interrupts_enabled();
	if ((p < allocator->base) || (p > allocator->end)) {
		return 0;
	}
	block = (generic_block_t *) ((p + allocator->base_addr) - sizeof(generic_block_t));
	if (block->state == MEMORY_GENERIC_IN_USE) {
		disable_interrupts();
		block->state = MEMORY_GENERIC_FREE;
		//destroy(block);
		interrupts_restore(save);
		return 1;
	}
	return 0;
}

void * allocators_malloc(allocator_t * allocator, size_t size) {
	return allocator->malloc(allocator, size);
}

int allocators_free(allocator_t * allocator, void * p) {
	return allocator->free(allocator, p);
}

allocator_t * allocators_create(uint16_t * name, void * base, uint32_t size) {
	allocator_t * allocator = malloc(sizeof(allocator_t));
	allocator->size = size;
	allocator->base = base;
	allocator->base_addr = (uint32_t) base;
	allocator->end = base + size;
	allocator->malloc = generic_malloc;
	allocator->free = generic_free;
	return allocator;
}

void allocators_init() {

}