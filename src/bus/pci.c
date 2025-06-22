#include <bus/pci.h>
#include <linked.h>
#include <memory.h>
#include <stdint.h>
#include <ports.h>

linked_t * pci_devices;
linked_t * pci_handlers;

void pci_add(pci_t * device) {
	pci_devices = linked_add(pci_devices, device);
}

void pci_add_handler(pci_callback_t callback, pci_callback_t check) {
	pci_handler_t * handler = malloc(sizeof(pci_handler_t));
	handler->callback = callback;
	handler->check = check;
	pci_handlers = linked_add(pci_handlers, handler);
}

int pci_exists(uint8_t bus, uint8_t slot, uint8_t function, uint16_t vendor, uint16_t device) {
	linked_t * node = pci_devices;
	while (node->next) {
		pci_t * pci_device = (pci_t *) node->p;
		if (pci_device->bus == bus && pci_device->slot == slot && pci_device->function == function && pci_device->vendor == vendor && pci_device->device == device) {
			return 1;
		}
		node = node->next;
	}
	return 0;
}

pci_t * pci_get(uint8_t bus, uint8_t slot, uint8_t function) {
	linked_t * node = pci_devices;
	while (node->next) {
		pci_t * pci_device = (pci_t *) node->p;
		if (pci_device->bus == bus && pci_device->slot == slot && pci_device->function == function) {
			return pci_device;
		}
		node = node->next;
	}
	return 0;
}

void pci_config_set_addr(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
	uint32_t address = 0x80000000
		| (offset  & 0xf00) << 16
		| (bus  & 0xff)  << 16
		| (slot  & 0x1f)  << 11
		| (function & 0x07)  << 8
		| (offset  & 0xfc);
	outd(0xcf8, address);
}

uint8_t pci_config_inb(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
	pci_config_set_addr(bus, slot, function, offset);
	return ind(0xcfc) >> ((offset & 3) * 8);
}

uint16_t pci_config_inw(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
	pci_config_set_addr(bus, slot, function, offset);
	return ind(0xcfc) >> ((offset & 3) * 8);
}

uint32_t pci_config_ind(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
	pci_config_set_addr(bus, slot, function, offset);
	return ind(0xcfc);
}


void pci_config_outb(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint8_t d) {
	uint32_t new = pci_config_ind(bus, slot, function, offset);
	new << ((offset & 3) * 8);
	new |= d << ((offset & 3) * 8);

	pci_config_set_addr(bus, slot, function, offset);
	outd(0xcfc, new);
}

void pci_config_outw(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint16_t d) {
	uint32_t new = pci_config_ind(bus, slot, function, offset);
	new << ((offset & 3) * 8);
	new |= d << ((offset & 3) * 8);

	pci_config_set_addr(bus, slot, function, offset);
	outd(0xcfc, new);
}

void pci_config_outd(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t d) {
	pci_config_set_addr(bus, slot, function, offset);
	outd(0xcfc, d);
}

void pci_cmd_set_flags(pci_t * device, uint32_t cmd) {
	uint16_t c = pci_config_inw(device->bus, device->slot, device->function, 0x4);
	c |= cmd;
	pci_config_outw(device->bus, device->slot, device->function, 0x4, c);
}

uint32_t pci_find_iobase(pci_t * device) {
	for (int i = 0; i < 6; i++) {
		uint16_t iobase = pci_config_ind(device->bus, device->slot, device->function, 0x10 + (i * 4));
		if (!iobase) {
			continue;
		}
		if (iobase & 1) {
			return iobase & 0xfffc;
		}
	}
	return 0;
}

uint8_t pci_get_irq(pci_t * pci) {
	return pci_config_inb(pci->bus, pci->slot, pci->function, 0x3c);
}

int pci_node_destructor(linked_t * node) {
	free(node->p);
}

int pci_call_handlers(linked_t * node, void * p) {
	pci_t * device = p;
	pci_handler_t * handler = node->p;
	if (handler->check(device)) {
		handler->callback(device);
	}
}

void pci_sync(pci_t * pci_device, uint32_t bus, uint32_t slot, uint32_t function) {
	uint16_t vendor = pci_config_inw(bus, slot, function, 0);
	uint16_t device = pci_config_inw(bus, slot, function, 2);
	uint16_t class = pci_config_inw(bus, slot, function, 0x0a);
	pci_device->vendor = vendor;
	pci_device->device = device;
	pci_device->class = (class >> 8) & 0xff;
	pci_device->subclass = class & 0xff;
	pci_device->function = function;
	pci_device->interface = pci_config_inb(bus, slot, function, 0x09);
	pci_device->slot = slot;
	pci_device->bar0 = pci_config_ind(bus, slot, function, 0x10);
	pci_device->bar1 = pci_config_ind(bus, slot, function, 0x14);
	pci_device->bar2 = pci_config_ind(bus, slot, function, 0x18);
	pci_device->bar3 = pci_config_ind(bus, slot, function, 0x1c);
	pci_device->bar4 = pci_config_ind(bus, slot, function, 0x20);
	pci_device->bar5 = pci_config_ind(bus, slot, function, 0x24);
}

void pci_resync(pci_t * pci) {
	pci_sync(pci, pci->bus, pci->slot, pci->function);
}

void pci_probe() {
	for (uint32_t bus = 0; bus < 256; bus++) {
		for (uint32_t slot = 0; slot < 32; slot++) {
			for (uint32_t function = 0; function < 8; function++) {
				uint16_t vendor = pci_config_inw(bus, slot, function, 0);
				if (vendor == 0xffff) {
					continue;
				}
				pci_t * device = malloc(sizeof(pci_t));
				pci_sync(device, bus, slot, function);
				pci_add(device);
				linked_iterate(pci_handlers, pci_call_handlers, device);
			}
		}
	}
	// we dont need this anymore
	linked_destroy_all(pci_handlers, pci_node_destructor, NULL);
}
