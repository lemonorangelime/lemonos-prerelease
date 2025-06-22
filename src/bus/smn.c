// system management network - dont fuck with this btw

#include <stdint.h>
#include <bus/pci.h>

uint32_t smn_read(uint32_t address) {
	pci_config_outd(0, 0, 0, 0x60, address);
	return pci_config_ind(0, 0, 0, 0x64);
}

void smn_write(uint32_t address, uint32_t d) {
	pci_config_outd(0, 0, 0, 0x60, address);
	pci_config_outd(0, 0, 0, 0x64, d);
}
