#pragma once

#include <stdint.h>

typedef struct {
	uint16_t vid;
	uint16_t * name;
} tpm_vendor_t;

typedef struct {
	uint16_t vid;
	uint16_t did;
	uint16_t * name;
} tpm_dev_t;

extern tpm_vendor_t tpm_vendors[];
extern tpm_dev_t tpm_devs[];

uint16_t tpm_search_vid(uint16_t * name);
uint16_t * tpm_search_vendor(uint16_t vid);
uint16_t * tpm_search_device(uint16_t vid, uint16_t did);