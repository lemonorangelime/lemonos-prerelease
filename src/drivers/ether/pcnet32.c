#include <drivers/ether/pcnet32.h>
#include <bus/pci.h>
#include <pit.h>
#include <math.h>
#include <stdio.h>
#include <ports.h>
#include <net/net.h>
#include <string.h>
#include <interrupts/irq.h>
#include <util.h>
#include <memory.h>

// this doesnt work and is a mess

int pcnet32_found = 0;
uint32_t pcnet32_base = 0;
uint32_t pcnet32_memory = 0;

int pcnet32_sending = 0;
int pcnet32_sent = 0;
uint8_t pcnet32_mode = 0;
int pcnet32_working = 0;

pcnet32_recv_descriptor_t * pcnet32_recv_descriptors = (void *) PCNET32_RECV_DESCRIPTORS;
pcnet32_trans_descriptor_t * pcnet32_trans_descriptors = (void *) PCNET32_TRANSMIT_DESCRIPTORS;

int pcnet32_current_recv = 0;
int pcnet32_current_trans = 0;
int pcnet32_prev_trans = 0;

uint32_t pcnet32_recv_buffer = PCNET32_RECV;
uint32_t pcnet32_trans_buffer = PCNET32_TRANSMIT;

pcnet32_init_t * pcnet32_init_struct = (void *) PCNET32_INIT;

interface_t * pcnet32_interface;

int pcnet32_init_done = 0;

// port access
void pcnet32_outw(uint16_t reg, uint16_t data) {
	if (pcnet32_memory != 0) {
		uint16_t * space = ((void *) pcnet32_memory) + reg;
		*space = data;
		return;
	}
	outw(pcnet32_base + reg, data);
}

void pcnet32_outd(uint16_t reg, uint16_t data) {
	if (pcnet32_memory != 0) {
		uint32_t * space = ((void *) pcnet32_memory) + reg;
		*space = data;
		return;
	}
	outd(pcnet32_base + reg, data);
}

uint8_t pcnet32_inb(uint16_t reg) {
	if (pcnet32_memory != 0) {
		uint8_t * space = ((void *) pcnet32_memory) + reg;
		return *space;
	}
	return inb(pcnet32_base + reg);
}

uint16_t pcnet32_inw(uint16_t reg) {
	if (pcnet32_memory != 0) {
		uint16_t * space = ((void *) pcnet32_memory) + reg;
		return *space;
	}
	return inw(pcnet32_base + reg);
}

uint32_t pcnet32_ind(uint16_t reg) {
	if (pcnet32_memory != 0) {
		uint32_t * space = ((void *) pcnet32_memory) + reg;
		return *space;
	}
	return ind(pcnet32_base + reg);
}


void pcnet32_write32_rap(uint32_t data) {
	pcnet32_outd(PCNET32_32_RAP, data);
}

void pcnet32_write16_rap(uint16_t data) {
	pcnet32_outw(PCNET32_16_RAP, data);
}

// register data port
void pcnet32_write32_rdp(uint32_t data) {
	pcnet32_outd(PCNET32_32_RDP, data);
}

void pcnet32_write16_rdp(uint16_t data) {
	pcnet32_outw(PCNET32_32_RDP, data);
}
uint32_t pcnet32_read32_rdp() {
	return pcnet32_ind(PCNET32_32_RDP);
}

uint16_t pcnet32_read16_rdp() {
	return pcnet32_inw(PCNET32_32_RDP);
}

void pcnet32_write32_bdp(uint32_t data) {
	pcnet32_outd(PCNET32_32_BDP, data);
}

void pcnet32_write16_bdp(uint16_t data) {
	pcnet32_outw(PCNET32_32_BDP, data);
}
uint32_t pcnet32_read32_bdp() {
	return pcnet32_ind(PCNET32_32_BDP);
}

uint16_t pcnet32_read16_bdp() {
	return pcnet32_inw(PCNET32_32_BDP);
}

// csr
void pcnet32_write32_csr(uint16_t csr, uint32_t data) {
	pcnet32_write32_rap(csr);
	pcnet32_write32_rdp(data);
}

void pcnet32_write16_csr(uint16_t csr, uint16_t data) {
	pcnet32_write16_rap(csr);
	pcnet32_write16_rdp(data);
}

uint32_t pcnet32_read32_csr(uint16_t csr) {
	pcnet32_write32_rap(csr);
	return pcnet32_read32_rdp();
}

uint16_t pcnet32_read16_csr(uint16_t csr) {
	pcnet32_write16_rap(csr);
	return pcnet32_read16_rdp();
}

uint32_t pcnet32_or32_csr(uint16_t csr, uint32_t bits) {
	pcnet32_write32_csr(csr, pcnet32_read32_csr(csr) | bits);
}

uint32_t pcnet32_or16_csr(uint16_t csr, uint16_t bits) {
	pcnet32_write16_csr(csr, pcnet32_read16_csr(csr) | bits);
}

uint32_t pcnet32_xor32_csr(uint16_t csr, uint32_t bits) {
	uint32_t value = pcnet32_read32_csr(csr);
	value ^= value & bits;
	pcnet32_write32_csr(csr, value);
}

uint32_t pcnet32_xor16_csr(uint16_t csr, uint16_t bits) {
	uint16_t value = pcnet32_read16_csr(csr);
	value ^= value & bits;
	pcnet32_write16_csr(csr, value);
}

// bcr
void pcnet32_write32_bcr(uint16_t bcr, uint32_t data) {
	pcnet32_write32_rap(bcr);
	pcnet32_write32_bdp(data);
}

void pcnet32_write16_bcr(uint16_t bcr, uint16_t data) {
	pcnet32_write16_rap(bcr);
	pcnet32_write16_bdp(data);
}

uint32_t pcnet32_read32_bcr(uint16_t bcr) {
	pcnet32_write32_rap(bcr);
	return pcnet32_read32_bdp();
}

uint16_t pcnet32_read16_cr(uint16_t bcr) {
	pcnet32_write16_rap(bcr);
	return pcnet32_read16_bdp();
}

void pcnet32_advance_recv_descriptor() {
	pcnet32_current_recv++;
	pcnet32_current_recv &= 0b11111;
}

void pcnet32_advance_trans_descriptor() {
	pcnet32_prev_trans = pcnet32_current_trans;
	pcnet32_current_trans++;
	pcnet32_current_recv &= 0b111;
}

uint16_t pcnet32_length_encode(uint16_t length) {
	return 0 - length;
}

void pcnet32_send_packet(void * packet, size_t size) {
	pcnet32_trans_descriptor_t * descriptor = &pcnet32_trans_descriptors[pcnet32_current_trans];
	memcpy((void *) ntohd(descriptor->address), packet, size);
	descriptor->length = htonw(pcnet32_length_encode(size));
	descriptor->status = htonw(0x8300);
	descriptor->misc = 0;
	pcnet32_advance_trans_descriptor();
	pcnet32_write32_csr(PCNET32_CSR_STATUS, PCNET32_STATUS_ENABLE_IRQ | PCNET32_STATUS_DEMAND);
}

void pcnet32_handle_packet() {
	pcnet32_recv_descriptor_t * descriptor = &pcnet32_recv_descriptors[pcnet32_current_recv];
	while (((int16_t) ntohw(descriptor->status)) >= 0) {
		if ((ntohw(descriptor->status) >> 8) == 3) {
			size_t size = ntohw(descriptor->length) & 0xfff;
			void * packet = (void *) ntohd(descriptor->address);

			net_handle_packet(pcnet32_interface, packet, size);
		}
		descriptor->status = htonw(0x8000);
		descriptor->buffer_length = htonw(pcnet32_length_encode(1520) & 0xfff);
		pcnet32_advance_recv_descriptor();
		descriptor = &pcnet32_recv_descriptors[pcnet32_current_recv];
	}
}

void pcnet32_callback(registers_t * regs) {
	uint32_t status = pcnet32_read32_csr(PCNET32_CSR_STATUS);
	pcnet32_handle_packet();
	pcnet32_or32_csr(PCNET32_CSR_STATUS, PCNET32_STATUS_RECEIVE_DONE);
}

void pcnet32_enable() {
}

void pcnet32_disable() {
}

void pcnet32_trans_descriptor_init(pcnet32_trans_descriptor_t * descriptor) {
	memset(descriptor, 0, sizeof(pcnet32_trans_descriptor_t));

	descriptor->address = 0;
	descriptor->length = 0;
	descriptor->status = 0;
}

void pcnet32_recv_descriptor_init(pcnet32_recv_descriptor_t * descriptor, uint32_t buffer) {
	memset(descriptor, 0, sizeof(pcnet32_recv_descriptor_t));

	descriptor->address = htond(buffer);
	descriptor->buffer_length = htonw(pcnet32_length_encode(1520) & 0xfff);
	descriptor->status = htonw(0x8000);
}

int pcnet32_reset(pci_t * pci) {
	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, pcnet32_callback);

	pcnet32_base = pci->bar0;

	pcnet32_ind(PCNET32_32_RESET);
	pcnet32_inw(PCNET32_16_RESET);

	uint64_t target = ticks + (pit_freq / 10);
	while (ticks < target) {}
	pcnet32_outd(0x10, 0);

	uint64_t mac = pcnet32_read_mac();
	uint32_t style = pcnet32_read32_csr(PCNET32_CSR_STYLE);
	style = (style & 0xff00) | PCNET32_STYLE_32;
	pcnet32_write32_csr(PCNET32_CSR_STYLE, style);

	pcnet32_write32_bcr(2, pcnet32_read32_bcr(2) | 2);
	pcnet32_write32_bcr(20, 2);

	for (int i = 0; i < 32; i++) {
		pcnet32_recv_descriptor_init(&pcnet32_recv_descriptors[i], pcnet32_recv_buffer + (1520 * i));
	}

	for (int i = 0; i < 8; i++) {
		pcnet32_trans_descriptor_init(&pcnet32_trans_descriptors[i]);
	}

	// GRAHHHHHHH
	memset(pcnet32_init_struct, 0, sizeof(pcnet32_init_t));
	pcnet32_init_struct->mode = htonw(3);
	pcnet32_init_struct->lengths = htonw((3 << 12) | (5 << 4));
	pcnet32_init_struct->filter = 0;
	pcnet32_init_struct->receive_descriptors = htond((uint32_t) pcnet32_recv_descriptors);
	pcnet32_init_struct->transmit_descriptors = htond((uint32_t) pcnet32_trans_descriptors);
	memcpy(pcnet32_init_struct->mac, &mac, 6);

	pcnet32_write32_csr(PCNET32_CSR1, PCNET32_INIT & 0xffff);
	pcnet32_write32_csr(PCNET32_CSR2, (PCNET32_INIT >> 16) & 0xffff);
	pcnet32_write32_csr(PCNET32_CSR_FEATURES, 0b0000100100010101);
	pcnet32_or32_csr(PCNET32_CSR_STATUS, PCNET32_STATUS_START_INIT);

	int timeout = 1;
	target = ticks + pit_freq;
	while (ticks < target) {
		if ((pcnet32_read32_csr(PCNET32_CSR_STATUS) & PCNET32_STATUS_INIT_DONE) != 0) {
			timeout = 0;
			break;
		}
	}

	if (timeout) {
		return 1;
	}

	//pcnet32_write32_csr(PCNET32_CSR_STATUS, PCNET32_STATUS_START_CARD | PCNET32_STATUS_ENABLE_IRQ);
	pcnet32_or32_csr(PCNET32_CSR_STATUS, PCNET32_STATUS_ENABLE_IRQ | PCNET32_STATUS_ENABLE_RECEIVE | PCNET32_STATUS_ENABLE_TRANSMIT);

	char * test_packet = "pcnet32testpacket";
	pcnet32_send_packet(test_packet, 32);
	return 0;
}

int pcnet32_check(pci_t * pci) {
	if (pci->vendor == 0x1022 && pci->device == 0x2000) {
		return 1; // Am79C970A
	}
	if (pci->vendor == 0x1022 && pci->device == 0x2001) {
		return 1; // Am79C970A but ISA edition (?)
	}
	return 0;
}

void pcnet32_send(interface_t * interface, void * packet, size_t size) {
	if (interface->mode == MODE_OFF || interface->working == 0) {
		return;
	}
	pcnet32_send_packet(packet, size);
}

uint8_t pcnet32_decode_mode(int mode) {
	switch (mode) {
		default:
			return pcnet32_mode;
		case MODE_OFF:
			return 0;
		case MODE_MONITOR:
			return 0; // all
		case MODE_MANAGED:
			return 0;
	}
}

int pcnet32_setmode(int mode) {
	return 0;
}

int pcnet32_ctrl(interface_t * interface, int cmd, uint32_t op) {
	if (cmd == NET_CMD_ENABLE) {
		pcnet32_enable();
		interface->working = 1;
		return 0;
	}
	if (interface->mode == MODE_OFF || interface->working == 0) {
		return -1;
	}
	uint64_t mac;
	switch (cmd) {
		case NET_CMD_SET_MODE:
			return pcnet32_setmode(op);
		case NET_CMD_SET_MAC_LOW:
			mac = interface->mac & 0xffffffff00000000;
			mac |= (uint64_t) op;
			return pcnet32_setmac(mac);
		case NET_CMD_SET_MAC_HIGH:
			mac = interface->mac & 0x00000000ffffffff;
			mac |= (uint64_t) op << 32;
			return pcnet32_setmac(mac);
		case NET_CMD_SET_IP: // dont send this to me !
			interface->ip = op;
			return 0;
		case NET_CMD_ELECT: // or this !
			interface->main = 1;
			net_get_default()->main = 0;
			return 0;
		case NET_CMD_SET_SPEED:
			return -1;
		case NET_CMD_DISABLE:
			pcnet32_disable();
			interface->working = 0;
			return 0;
		case NET_CMD_CLEAR_ROUTES: // i cant do this for you !!!!!
		case NET_CMD_SET_GATEWAY: // i dont want to do this for you !
			return -1;
		case NET_CMD_SET_IDENTITY: // please !
			interface->identity = op;
			return 0;
		case NET_CMD_CLEAR_STATS: // why !
			interface->stat_byte_recv = 0;
			interface->stat_byte_sent = 0;
			interface->stat_sent = 0;
			interface->stat_recv = 0;
			return 0;
	}
	return -1;
}

uint64_t pcnet32_read_mac() {
	uint64_t mac = 0;
	for (int i = 0; i < 6; i++) {
		mac |= pcnet32_inb(PCNET32_32_EEPROM + i) << (8 * i);
	}
	return mac;
}

int pcnet32_setmac(uint64_t mac) {
	return 0;
}

int pcnet32_dev_init(pci_t * pci) {
	pcnet32_found++;
	if (pcnet32_found != 1) {
		return 0;
	}

	pci_cmd_set_flags(pci, PCI_CMD_MASTER | PCI_CMD_IO | PCI_CMD_MEM); // enable dma, IO, and memory
	if (pcnet32_reset(pci)) {
		return 0;
	}

	pcnet32_interface = net_create_interface(u"pcnet32", NULL, pcnet32_send, pcnet32_ctrl, IF_ETHER, 0);
	pcnet32_interface->working = 1;
	pcnet32_interface->mode = MODE_MANAGED;
	pcnet32_interface->speed = 10;
	pcnet32_interface->mac = pcnet32_read_mac();
	pcnet32_setmac(pcnet32_interface->mac);
	net_register_interface(pcnet32_interface);
	pcnet32_working = 1;
}

void pcnet32_init() {
	if (!network_enabled) {
		return;
	}
	pci_add_handler(pcnet32_dev_init, pcnet32_check);
}
