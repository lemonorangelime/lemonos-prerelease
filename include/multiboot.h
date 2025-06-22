#pragma once

#include <stdint.h>

typedef struct multiboot_header {
	uint32_t flags;
	uint32_t memory_low;
	uint32_t memory_high;
	uint32_t boot_device;
	char * cmdline;
	uint32_t modules_count;
	uint32_t modules_address;
	uint64_t ignored1;
	uint64_t ignored2;
	uint32_t mmap_count;
	uint32_t mmap_address;
	uint32_t drives_count;
	uint32_t drives_address;
	uint32_t config;
	char * bootloader_name;
	uint32_t apm_table;
	uint32_t vbe_control;
	uint32_t vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;
	uint64_t framebuffer;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;
} multiboot_header_t;

typedef struct {
	unsigned int size;
	uint64_t address;
	uint64_t length;
	unsigned int type;
} __attribute__((packed)) memory_map_t;

typedef struct {
	void * start;
	void * end;
	char string[6];
} __attribute__((packed)) multiboot_module_t;

enum MMAP_TYPES {
	MMAP_AVAILABLE = 1,
	MMAP_RESERVED,
	MMAP_RECLAIMABLE,
	MMAP_NON_VOLATILE,
	MMAP_BAD,
};

int parse_multiboot(uint32_t eax, uint32_t ebx);
extern multiboot_header_t * multiboot_header;
