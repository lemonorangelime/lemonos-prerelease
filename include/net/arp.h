#pragma once

#include <stdint.h>

typedef struct {
	uint32_t address;
	uint64_t mac;
} arp_entry_t;

#define ARP_TABLE_SIZE 32

uint64_t arp_resolve(uint32_t address);
uint64_t arp_lookup(uint32_t address);
void arp_init();