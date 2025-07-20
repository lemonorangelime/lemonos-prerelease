#include <drivers/ether/ne2k.h>
#include <net/net.h>
#include <string.h>
#include <bus/pci.h>
#include <graphics/graphics.h>
#include <ports.h>
#include <interrupts/irq.h>
#include <stdio.h>
#include <cpuspeed.h>
#include <memory.h>

// this doesnt work

int ne2k_found = 0;
int ne2k_working = 0;
uint8_t ne2k_oldcmd = 0;
uint16_t ne2k_iobase = 0;
interface_t * ne2k_interface;

void ne2k_send_packet(void * packet, size_t size) {
	ne2k_oldcmd = NE2K_CMD_PAGE0;
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0);
	outb(ne2k_iobase + NE2K_P0W_IRQ_MASK, 0);

	ne2k_dma_out(packet, 0x4000, size);
}

void ne2k_send(interface_t * interface, void * packet, size_t size) {
	if (interface->mode == MODE_OFF || interface->working == 0) {
		return;
	}
	ne2k_send_packet(packet, size);
}

void ne2k_enable() {
}

void ne2k_disable() {
}

int ne2k_setmode(int mode) {
	return 0;
}

int ne2k_setmac(uint64_t mac) {
	return 0;
}

int ne2k_ctrl(interface_t * interface, int cmd, uint32_t op) {
	if (cmd == NET_CMD_ENABLE) {
		ne2k_enable();
		interface->working = 1;
		return 0;
	}
	if (interface->mode == MODE_OFF || interface->working == 0) {
		return -1;
	}
	uint64_t mac;
	switch (cmd) {
		case NET_CMD_SET_MODE:
			return ne2k_setmode(op);
		case NET_CMD_SET_MAC_LOW:
			mac = interface->mac & 0xffffffff00000000;
			mac |= (uint64_t) op;
			return ne2k_setmac(mac);
		case NET_CMD_SET_MAC_HIGH:
			mac = interface->mac & 0x00000000ffffffff;
			mac |= (uint64_t) op << 32;
			return ne2k_setmac(mac);
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
			ne2k_disable();
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

uint64_t ne2k_read_mac() {
}

void ne2k_dma_in(void * p, uint16_t frame, uint16_t size) {
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_RDMA_ABORT | NE2K_CMD_PAGE0 | NE2K_CMD_START);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_COUNT_LOW, size & 0xff);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_COUNT_HIGH, (size >> 8) & 0xff);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_DMA_LOW, frame & 0xff);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_DMA_HIGH, (frame >> 8) & 0xff);
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_RDMA_READ | NE2K_CMD_START);

	uint16_t * p16 = p;
	int header = size == 4;
	while (size > 1) {
		*p16 = inw(ne2k_iobase + NE2K_DATA);
		p16++;
		size -= 2;
	}

	if (size) {
		uint8_t * p8 = (void *) p16;
		*p8 = inw(ne2k_iobase + NE2K_DATA);
	}

	outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, NE2K_IRQ_STAT_DMA_COMPLETE);
}

void ne2k_dma_out(void * p, uint16_t frame, uint16_t size) {
	size++;
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_RDMA_ABORT | NE2K_CMD_PAGE0 | NE2K_CMD_START);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_COUNT_LOW, size & 0xff);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_COUNT_HIGH, (size >> 8) & 0xff);
	outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, NE2K_IRQ_STAT_DMA_COMPLETE);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_DMA_LOW, frame & 0xff);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_DMA_HIGH, (frame >> 8) & 0xff);
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_RDMA_WRITE | NE2K_CMD_START);

	uint16_t * p16 = p;
	while (size > 1) {
		outw(ne2k_iobase + NE2K_DATA, *p16++);
		size -= 2;
	}

	if (size) {
		uint8_t * p8 = (void *) p16;
		outb(ne2k_iobase + NE2K_DATA, *p8);
	}
}

void ne2k_handle_packet() {
	uint8_t page = 0;
	uint8_t frame = 0;
	ne2k_packet_header_t header;
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_RDMA_ABORT | NE2K_CMD_PAGE1);
	page = inb(ne2k_iobase + NE2K_P1R_CURRENT_PAGE);
	outb(ne2k_iobase + NE2K_P1W_CMD, NE2K_CMD_RDMA_ABORT | NE2K_CMD_PAGE0);
	frame = inb(ne2k_iobase + NE2K_P0R_BOUNDRY_POINTER);

	ne2k_dma_in(&header, frame << 8, 4);
	uint16_t size = header.size - 4;
	void * packet = malloc(size);
	ne2k_dma_in(packet, frame << 8, size);

	net_handle_packet(ne2k_interface, packet, size);
	outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, NE2K_IRQ_STAT_RECEIVED | NE2K_IRQ_STAT_RECEIVE_ERROR);
}

void ne2k_restart() {
	int resend;

	cpuspeed_wait_tsc(cpu_hz / 10);

	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0 | NE2K_CMD_STOP);

	outb(ne2k_iobase + NE2K_P0W_REMOTE_COUNT_LOW, 0);
	outb(ne2k_iobase + NE2K_P0W_REMOTE_COUNT_HIGH, 0);
	
	resend = inb(ne2k_iobase + NE2K_P0R_IRQ_STAT) & (NE2K_IRQ_STAT_TRANSMITED | NE2K_IRQ_STAT_TRANSMIT_ERROR);

	outb(ne2k_iobase + NE2K_P0W_TRANSMIT_CONFIG, NE2K_TRANSMIT_LOOPBACK);
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0 | NE2K_CMD_START);

	ne2k_handle_packet();
	outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, NE2K_IRQ_STAT_BOUNDRY_REACHED);

	outb(ne2k_iobase + NE2K_P0W_TRANSMIT_CONFIG, 0);
	if (resend) {
		outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0 | NE2K_CMD_START | NE2K_CMD_TRANSMIT);
	}
}

void ne2k_callback(registers_t * regs) {
	outb(ne2k_iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0);

	uint8_t status = inb(ne2k_iobase + NE2K_P0R_IRQ_STAT);
	int i = 0;
	while (status != 0) {
		if (i == 1000) {
			break;
		}
		if (status & NE2K_IRQ_STAT_BOUNDRY_REACHED) {
			ne2k_restart();
		}
		if (status & (NE2K_IRQ_STAT_RECEIVED | NE2K_IRQ_STAT_RECEIVE_ERROR)) {
			ne2k_handle_packet();
		}
		if (status & (NE2K_IRQ_STAT_TRANSMITED | NE2K_IRQ_STAT_TRANSMIT_ERROR)) {
			outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, status & (NE2K_IRQ_STAT_TRANSMITED | NE2K_IRQ_STAT_TRANSMIT_ERROR));
		}
		if (status & (NE2K_IRQ_STAT_OVERFLOW | NE2K_IRQ_STAT_DMA_COMPLETE)) {
			outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, status & (NE2K_IRQ_STAT_OVERFLOW | NE2K_IRQ_STAT_DMA_COMPLETE));
		}
		status = inb(ne2k_iobase + NE2K_P0R_IRQ_STAT);
		i++;
	}

	outb(ne2k_iobase + NE2K_P0W_IRQ_STAT, 0xff); // ack all irq
	outb(ne2k_iobase + NE2K_P0W_CMD, ne2k_oldcmd);
}

void ne2k_reset(pci_t * pci, uint16_t iobase) {
	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, ne2k_callback);

	outb(iobase + NE2K_RESET, inb(iobase + NE2K_RESET));
	while ((inb(iobase + NE2K_P0R_IRQ_STAT) & NE2K_IRQ_STAT_RESET_STATUS) == 0) {}

	outb(iobase + NE2K_P0W_IRQ_STAT, 0xff); // ack all irq

	outb(iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0 | NE2K_CMD_STOP);
	outb(iobase + NE2K_P0W_DATA_CONFIG, NE2K_DATA_CONFIG_WORD | NE2K_DATA_CONFIG_NOLOOP | NE2K_DATA_CONFIG_FIFO_QWORD);
	outb(iobase + NE2K_P0W_REMOTE_COUNT_LOW, 0);
	outb(iobase + NE2K_P0W_REMOTE_COUNT_HIGH, 0);
	outb(iobase + NE2K_P0W_IRQ_MASK, 0);
	outb(iobase + NE2K_P0W_IRQ_STAT, 0xff);
	outb(iobase + NE2K_P0W_RECEIVE_CONFIG, NE2K_RECEIVE_DISABLE);
	outb(iobase + NE2K_P0W_TRANSMIT_CONFIG, NE2K_TRANSMIT_LOOPBACK);

	outb(iobase + NE2K_P0W_REMOTE_COUNT_LOW, 32);
	outb(iobase + NE2K_P0W_REMOTE_COUNT_HIGH, 0);
	outb(iobase + NE2K_P0W_REMOTE_DMA_LOW, 0);
	outb(iobase + NE2K_P0W_REMOTE_DMA_HIGH, 0);
	outb(iobase + NE2K_P0W_CMD, NE2K_CMD_START | NE2K_CMD_RDMA_READ);

	outb(iobase + NE2K_P0W_TRANSMIT_PAGE_START, 0x40);
	outb(iobase + NE2K_P0W_PAGE_START, 0x40 + 12);
	outb(iobase + NE2K_P0W_BOUNDRY_POINTER, 0x80 - 1);
	outb(iobase + NE2K_P0W_PAGE_STOP, 0x80);

	outb(iobase + NE2K_P0W_IRQ_STAT, 0xff);
	outb(iobase + NE2K_P0W_IRQ_MASK, 0x3f);
	
	outb(iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0 | NE2K_CMD_START);

	outb(iobase + NE2K_P0W_CMD, NE2K_CMD_PAGE0 | NE2K_CMD_START);
	outb(iobase + NE2K_P0W_TRANSMIT_CONFIG, 0);
	outb(iobase + NE2K_P0W_RECEIVE_CONFIG, NE2K_RECEIVE_ACCEPT_CRC | NE2K_RECEIVE_ACCEPT_SMALL | NE2K_RECEIVE_ACCEPT_BROADCAST | NE2K_RECEIVE_ACCEPT_PHYSICALS);
}

int ne2k_dev_init(pci_t * pci) {
	ne2k_found++;
	if (ne2k_found != 1) {
		return 0;
	}
	
	pci_cmd_set_flags(pci, PCI_CMD_MASTER | PCI_CMD_IO | PCI_CMD_MEM); // enable dma and IO and memory

	uint16_t iobase = pci->bar0 & ~3;
	ne2k_iobase = iobase;
	ne2k_reset(pci, iobase);

	ne2k_interface = net_create_interface(u"ne2k", NULL, ne2k_send, ne2k_ctrl, IF_ETHER, 0);
	ne2k_interface->working = 1;
	ne2k_interface->mode = MODE_MANAGED;
	ne2k_interface->speed = 10;
	ne2k_interface->mac = ne2k_read_mac();
	net_register_interface(ne2k_interface);

	printf(u"NE2K: detected Ne2000 ethernet controller\n");
}

int ne2k_unsafe_scan() {
	return 0;
}

int ne2k_check(pci_t * pci) {
	if (pci->vendor == 0x10ec && pci->device == 0x8029) {
		return 1; // RTL8029
	}
	if (pci->vendor == 0x1050 && pci->device == 0x0940) {
		return 1; // Winbond 89C940
	}
	if (pci->vendor == 0x11f6 && pci->device == 0x1401) {
		return 1; // Compex RL2000
	}
	if (pci->vendor == 0x8e2e && pci->device == 0x3000) {
		return 1; // KTI ET32P2
	}
	if (pci->vendor == 0x4a14 && pci->device == 0x5000) {
		return 1; // NetVin NV5000SC
	}
	if (pci->vendor == 0x10bd && pci->device == 0x0e34) {
		return 1; // SureCom NE34
	}
	if (pci->vendor == 0x1050 && pci->device == 0x5a5a) {
		return 1; // Winbond W89C940F
	}
	if (pci->vendor == 0x8c4a && pci->device == 0x1980) {
		return 1; // Winbond W89C940
	}
	return 0;
}

void ne2k_init() {
	if (!network_enabled) {
		return;
	}
	pci_add_handler(ne2k_dev_init, ne2k_check);

	if (ne2k_found == 0) {
		uint16_t iobase = ne2k_unsafe_scan();
		if (iobase) {
			ne2k_found = 1;
			ne2k_reset(NULL, iobase);
		}
	}
}
