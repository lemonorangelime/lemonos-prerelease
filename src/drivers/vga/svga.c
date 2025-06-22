// VMWare SVGA 2
#include <bus/pci.h>
#include <math.h>
#include <stdio.h>

// not finished obviouslly

void svga_reset(pci_t * pci) {
	uint16_t cmd = pci_config_inw(pci->bus, pci->slot, pci->function, 2);
	pci_config_outw(pci->bus, pci->slot, pci->function, 2, cmd | PCI_CMD_MASTER | PCI_CMD_IO | PCI_CMD_MEM);

	
}

int svga_check(pci_t * pci) {
	if (pci->vendor == 0x15ad && pci->device == 0x0405) {
		return 1; // VMware SVGA II
	}
	return 0;
}

static int svga_found = 0;
int svga_dev_init(pci_t * pci) {
	svga_found++;
	if (svga_found != 1) {
		cprintf(LEGACY_COLOUR_RED, u"Ignoring %d%s SVGA device\n", svga_found, number_text_suffix(svga_found));
		return 0;
	}
}

void svga_init() {
	pci_add_handler(svga_dev_init, svga_check);
}
