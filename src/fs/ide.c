#include <fs/ide.h>
#include <bus/pci.h>
#include <string.h>
#include <memory.h>
#include <ports.h>
#include <stdio.h>

// 

void ide_try_native(pci_t * pci) {
	uint8_t interface = pci->interface;
	uint8_t mutable = interface & 0b0010;
	if (!mutable) {
		return;
	}
	//printf(u"IDE: switching to native mode\n");
	pci_config_outb(pci->bus, pci->slot, pci->function, 0x09, interface | 1);
	pci_resync(pci);
}

uint8_t ide_() {}

void ide_reset(ide_t * ide) {
	pci_t * pci = ide->pci;
}

int ide_dev_init(pci_t * pci) {
	uint8_t interface = pci->interface;
	int native = (interface & 0b0001);
	//printf(u"IDE: found IDE controller (%s)\n", native ? u"native" : u"compatibility");
	if (!native) {
		//printf(u"IDE: %s switch to native mode\n", (interface & 0b0010) ? u"CAN" : u"can NOT");
	}
	ide_try_native(pci);
	//printf(u"IDE: DMA is %s\n", (interface & 0b10000000) ? u"supported" : u"unsupported");

	ide_t * device = malloc(sizeof(ide_t));
	device->native = native;
	device->dma = 0;
	device->bar0 = native ? pci->bar0 : 0x01f0;
	device->bar1 = native ? pci->bar1 : 0x03f6;
	device->bar2 = native ? pci->bar2 : 0x0170;
	device->bar3 = native ? pci->bar3 : 0x0376;
	device->bar4 = native ? pci->bar4 : 0;

	ide_reset(device);
}

int ide_check(pci_t * pci) {
	if (pci->class == 0x01 && pci->subclass == 0x01) {
		return 1;
	}
	return 0;
}

void ide_init() {
	pci_add_handler(ide_dev_init, ide_check);
}
