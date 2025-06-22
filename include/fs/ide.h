#pragma once

#include <stdint.h>
#include <bus/pci.h>

typedef struct {
	int native;
	int dma;
	pci_t * pci;
	uint32_t bar0;
	uint32_t bar1;
	uint32_t bar2;
	uint32_t bar3;
	uint32_t bar4;
} ide_t;

enum {
	IDE_A,
};

void ide_init();
