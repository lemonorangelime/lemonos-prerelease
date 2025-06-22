#pragma once

#include <stdint.h>

typedef struct {
	uint8_t type;
	uint8_t size;
	uint16_t handle;
} __attribute__((packed)) smbios_header_t;

typedef struct {
	char name[4];
	uint8_t checksum;
	uint8_t size;
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t max_size;
	uint8_t revision;
	char format[5];
	char intermediate[5];
	uint8_t intermediate_checksum;
	uint16_t total_size;
	smbios_header_t * table;
	uint16_t count;
	uint8_t bcd_revision;
} __attribute__((packed)) smbios_anchor_t;

typedef struct {
	smbios_anchor_t * anchor;
	smbios_header_t * header;
} smbios_iterator_t;

void smbios_init();