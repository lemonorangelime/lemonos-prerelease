#include <fs/scsi.h>
#include <fs/ide.h>
#include <bus/pci.h>
#include <string.h>
#include <memory.h>
#include <ports.h>
#include <stdio.h>
#include <util.h>
#include <fs/drive.h>
#include <interrupts/irq.h>
#include <cpuspeed.h>
#include <string.h>
#include <bus/pcidevs.h>

void scsi_reset() {
}

int scsi_dev_init(pci_t * pci) {
	uint16_t iobase = pci_find_iobase(pci);
	uint16_t * vendorname = pcidev_search_vendor(pci->vendor);
	uint16_t * devicename = pcidev_search_device(pci->vendor, pci->device);
	printf(u"SCSI: %s @ %r\n", devicename, iobase);
	printf(u"SCSI: status @ %r\n", inb(iobase + 0b001));
}

int scsi_check(pci_t * pci) {
	if (pci->class == 0x01 && pci->subclass == 0x00) {
		return 1;
	}
	return 0;
}

void scsi_init() {
	pci_add_handler(scsi_dev_init, scsi_check);
}