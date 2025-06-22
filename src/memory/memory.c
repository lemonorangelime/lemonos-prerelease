#include <memory.h>
#include <math.h>
#include <multiboot.h>
#include <string.h>
#include <panic.h>
#include <util.h>
#include <boot/initrd.h>
#include <input/input.h>
#include <multitasking.h>

// awful code but it works

void * heap;
void * heap_end;
int memory_alignment = 16;

size_t ram_size = 0;
size_t heap_length = 0;
size_t used_memory = 0;

void mmap_parse() {
	size_t i = 0;
	heap_length = 0;
	heap = NULL;
	heap_end = NULL;
	for (i = 0; i < multiboot_header->mmap_count; i += sizeof(memory_map_t)) {
		memory_map_t * mmap = (memory_map_t *) (multiboot_header->mmap_address + i);
		if (mmap->type == MMAP_BAD || mmap->type == MMAP_NON_VOLATILE) {
			continue;
		}
		if (mmap->length > heap_length) {
			heap = (void *) (uint32_t) mmap->address;
			heap_length = mmap->length;

			// cut it off if its longer than 0xffffffff
			if (heap_length > (0xffffffff - mmap->address)) {
				heap_length = (0xffffffff - mmap->address);
			}
		}
	}
	ram_size = heap_length;
	heap_end = heap + heap_length;
	if (((uint32_t) heap) == 0x100000) {
		void * kernel_end = (void *) round32((uint32_t) &_kernel_end, memory_alignment);
		used_memory += (uint32_t) initrd_module->end;
		heap = initrd_module->end;
		heap_length = ((uint32_t) heap_end) - ((uint32_t) heap);
	}
	heap = (void *) round32((uint32_t) heap, memory_alignment) + 8;
}

static int destroy(memory_block_t * block) {
	void * p = block;
	memory_block_t * next = p + block->size + sizeof(memory_block_t);
	if (next->state != MEMORY_NON_EXISTENT) {
		return 0; // sowwy
	}
	block->state = MEMORY_NON_EXISTENT;
	block->size = 0;
	return 1;
}

// split to desired size, free created block
// - block HAS to be exactly or bigger than the desired size or underallocation will occur
static int split(memory_block_t * block, size_t size) {
	uint32_t old;
	void * p = block;
	memory_block_t * new_block;
	if (block->size < (size + 8 + sizeof(memory_block_t))) {
		return 1;
	}
	old = block->size;
	block->size = size;
	new_block = p + size + sizeof(memory_block_t);
	new_block->state = MEMORY_FREE;
	new_block->size = old - size - sizeof(memory_block_t);
	destroy(new_block);
	return 1;
}

// physical malloc
void * malloc(size_t size) {
	size_t required_size = round32(size, memory_alignment); // round to memory_alignment
	memory_block_t * current_block = (memory_block_t *) heap;
	void * current = heap;
	uint64_t * next_block;
	size_t consecutive_free = 0;
	void * consecutive_start = 0;
	int save = interrupts_enabled();
	if ((required_size % 8) == 0) {
		required_size += 8;
	}
	if ((size == 0) || ((heap + required_size + sizeof(memory_block_t)) > heap_end)) {
		return NULL;
	}
	disable_interrupts();
	while ((current_block->state == MEMORY_FREE) || (current_block->state == MEMORY_IN_USE)) {
		if (current_block->state == MEMORY_FREE) {
			if (current_block->size >= required_size) {
				split(current_block, required_size);
				current_block->state = MEMORY_IN_USE;
				interrupts_restore(save);
				used_memory += current_block->size + 8;
				return current + sizeof(memory_block_t);
			}
			memory_block_t * next = current + current_block->size + sizeof(memory_block_t);
			if (next->state == MEMORY_NON_EXISTENT) {
				break;
			}
			consecutive_free += current_block->size + sizeof(memory_block_t);
			if (!consecutive_start) {
				consecutive_start = current;
			}
			if ((consecutive_free - sizeof(memory_block_t)) >= required_size) {
				consecutive_free -= sizeof(memory_block_t);
				current_block = consecutive_start;
				current_block->size = consecutive_free;
				split(current_block, required_size);
				current_block->state = MEMORY_IN_USE;
				interrupts_restore(save);
				used_memory += current_block->size + 8;
				return consecutive_start + sizeof(memory_block_t);
			}
		} else {
			consecutive_free = 0;
			consecutive_start = 0;
		}
		current += current_block->size + sizeof(memory_block_t);
		current_block = (memory_block_t *) current;
		if (current > heap_end || current_block->state > 3) {
			handle_error(MEMORY_CORRUPTION, 0);
			interrupts_restore(save);
			return NULL;
		}
	}
	// prevent us from getting confused by left over memory (see memory_init())
	next_block = ((void *) current_block) + required_size + sizeof(memory_block_t);
	*next_block++ = 0;
	*next_block++ = 0;
	*next_block++ = 0;
	*next_block++ = 0;
	// create new block
	current_block->state = MEMORY_IN_USE;
	current_block->size = required_size;
	used_memory += required_size + 8;
	interrupts_restore(save);
	return ((void *) current_block) + sizeof(memory_block_t);
}

int free(void * data) {
	memory_block_t * block;
	int save;
	if ((data < heap) || (data > heap_end)) {
		return 0;
	}
	block = (memory_block_t *) (data - sizeof(memory_block_t));
	if (block->state == MEMORY_IN_USE) {
		save = interrupts_enabled();
		disable_interrupts();
		used_memory -= block->size + 8;
		block->state = MEMORY_FREE;
		destroy(block);
		interrupts_restore(save);
		return 1;
	}
	return 0;
}

void * realloc(void * p, size_t size) {
	memory_block_t * block;
	if (!size) {
		free(p);
		return 0;
	}
	block = (memory_block_t *) (p - sizeof(memory_block_t));
	if (block->size >= size) {
		return p;
	}
	free(p);
	return malloc(size);
}

void * calloc(size_t number, size_t size) {
	void * p = malloc(number * size);
	if (!p) {
		return p;
	}
	memset(p, 0, number * size);
	return p;
}

void memory_init() {
	uint64_t * p2;
	used_memory = 0;
	mmap_parse();
	// memory doesnt get wipped on reboot, this will prevent us from getting confused by stuff left from last boot
	// (see malloc() for next half of the solution)
	p2 = heap;
	*p2 = 0;

	char * identifier = malloc(16);
	memcpy(identifier, "__phymem__", 16); // buffer overread, but who care
}

void memory_late_init() {
	return;
}
