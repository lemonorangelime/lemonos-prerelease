#pragma once

#include <stdint.h>

typedef struct {
	uint16_t vendor;
	uint16_t * name;
} pci_vendor_t;

typedef struct {
	uint16_t vendor;
	uint16_t device;
	uint16_t * name;
} pci_device_t;

typedef struct {
	uint8_t class;
	uint8_t subclass;
	uint16_t * name;
} pci_class_t;

uint16_t * pcidev_search_vendor(uint16_t vendor);
uint16_t * pcidev_search_device(uint16_t vendor, uint16_t device);

extern pci_vendor_t pci_vendor_ids[];
extern pci_device_t pci_device_ids[];