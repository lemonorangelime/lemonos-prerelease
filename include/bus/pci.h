#pragma once

#include <stdint.h>
#include <linked.h>

typedef struct {
	uint16_t vendor;
	uint16_t device;
	uint8_t function;
	uint8_t slot;
	uint8_t bus;
	uint8_t revision;
	uint8_t class;
	uint8_t subclass;
	uint8_t irq_line;
	uint8_t irq_pin;
	uint8_t latency;
	uint8_t grant;
	uint8_t interface;
	uint32_t bar0;
	uint32_t bar1;
	uint32_t bar2;
	uint32_t bar3;
	uint32_t bar4;
	uint32_t bar5;
	uint32_t id;
} pci_t;

typedef int (* pci_callback_t)(pci_t * device);

typedef struct {
	pci_callback_t callback;
	pci_callback_t check;
} pci_handler_t;

enum {
	PCI_CMD_IO       = 0b0000000000000001, // io space enable
	PCI_CMD_MEM      = 0b0000000000000010, // memory space enable
	PCI_CMD_MASTER   = 0b0000000000000100, // bus master enable
	PCI_CMD_CYCLE    = 0b0000000000001000, // special cycle enable
	PCI_CMD_WRITE    = 0b0000000000010000, // memory write enable
	PCI_CMD_SNOOPY   = 0b0000000000100000, // snoopy :3 (vga snoop enable)
	PCI_CMD_PARERROR = 0b0000000001000000, // parity error / disable
	PCI_CMD_SIGERROR = 0b0000000100000000, // signal error / enable
	PCI_CMD_FBtB     = 0b0000001000000000, // fast back to back / enable
	PCI_CMD_IRQ      = 0b0000010000000000, // irq enable / disable
};

extern linked_t * pci_devices;

void pci_add(pci_t * device);
void pci_add_handler(pci_callback_t handler, pci_callback_t check);
int pci_exists(uint8_t bus, uint8_t slot, uint8_t function, uint16_t vendor, uint16_t device);
pci_t * pci_get(uint8_t bus, uint8_t slot, uint8_t function);
uint16_t pci_config_inw(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
uint8_t pci_config_inb(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
uint32_t pci_config_ind(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
void pci_config_outd(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t d);
void pci_config_outw(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint16_t d);
void pci_config_outb(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint8_t d);
void pci_cmd_set_flags(pci_t * device, uint32_t cmd);
uint32_t pci_find_iobase(pci_t * device);
uint8_t pci_get_irq(pci_t * pci);
void pci_sync(pci_t * pci_device, uint32_t bus, uint32_t slot, uint32_t function);
void pci_resync(pci_t * pci);
void pci_probe();
void pci_init();