#include <drivers/ether/rtl8139.h>
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

// this works but is a mess

int rtl8139_found = 0;
uint32_t rtl8139_base = 0;
uint32_t rtl8139_memory = 0;

uint32_t rtl8139_rx_base = RTL8139_DMA;
uint32_t rtl8139_rx_offset = 0;
uint32_t rtl8139_rx_size = 8192;

uint32_t rtl8139_fifo_addrs[] = {0, 0, 0, 0};

int rtl8139_sending = 0;
int rtl8139_sent = 0;
uint8_t rtl8139_mode = 0;
int rtl8139_working = 0;

interface_t * rtl8139_interface;

void rtl8139_outb(uint16_t offset, uint8_t byte) {
	outb(rtl8139_base + offset, byte);
}

void rtl8139_outw(uint16_t offset, uint16_t byte) {
	outw(rtl8139_base + offset, byte);
}

void rtl8139_outd(uint16_t offset, uint32_t byte) {
	outd(rtl8139_base + offset, byte);
}

uint8_t rtl8139_inb(uint16_t offset) {
	return inb(rtl8139_base + offset);
}

uint16_t rtl8139_inw(uint16_t offset) {
	return inw(rtl8139_base + offset);
}

uint32_t rtl8139_ind(uint16_t offset) {
	return ind(rtl8139_base + offset);
}

void rtl8139_send_packet(void * packet, size_t size) {
	int offset = rtl8139_sending * 4;
	void * newpacket = malloc(size);
	memcpy(newpacket, packet, size);
	rtl8139_fifo_addrs[rtl8139_sending] = (uint32_t) newpacket;
	rtl8139_outd(REG_TXADDR + offset, (uint32_t) newpacket);
	rtl8139_outd(REG_TXSTAT + offset, size);
	rtl8139_sending++;
	rtl8139_sending %= 4;
}

void rtl8139_handle_packet() {
	uint8_t cmd = rtl8139_inb(REG_CMD);
	int i = 0;
	while ((cmd & CMD_EMPTYRX) == 0) {
		if (i > rtl8139_rx_size) {
			net_ctrl(rtl8139_interface->id, NET_CMD_DISABLE, 0);
			return;
		}
		uint32_t stat = *(uint32_t *) (rtl8139_rx_base + rtl8139_rx_offset);
		uint16_t length = stat >> 16;
		void * packet = (void *) (rtl8139_rx_base + rtl8139_rx_offset + 4);
		if (stat & (RX_PACKET_ALIGNMENT | RX_PACKET_CRC_ERR | RX_PACKET_LONG | RX_PACKET_SHORT | RX_PACKET_SYMBOL)) {
			rtl8139_outb(REG_CMD, CMD_ENABLE_TX);
			rtl8139_enable();
		} else {
			net_handle_packet(rtl8139_interface, packet, length - 4);
		}
		rtl8139_rx_offset += (length + 4 + 3) & 0xfffc; // get next packet
		if (rtl8139_rx_offset >= rtl8139_rx_size) {
			rtl8139_rx_offset -= rtl8139_rx_size;
		}
		rtl8139_outw(REG_RXTOP, rtl8139_rx_offset - 16); // tell card how much we have read so far
		cmd = rtl8139_inb(REG_CMD);
		i++;
	}
}

void rtl8139_callback(registers_t * regs) {
	uint16_t stat = rtl8139_inw(REG_INTSTAT);
	rtl8139_outw(REG_INTSTAT, 0x5);
	if (!rtl8139_working) {
		return;
	}
	if (stat & 0x1) {
		rtl8139_handle_packet();
	}
	if (stat & 0x4) {
		uint32_t addr = rtl8139_fifo_addrs[rtl8139_sent];
		free((void *) addr);
		rtl8139_sent++;
		rtl8139_sent %= 4;
	}
}

void rtl8139_enable() {
	rtl8139_outd(REG_RXCONF, rtl8139_mode | (1 << 7));
	rtl8139_outw(REG_INTMASK, INTMASK_RX | INTMASK_TX);
}

void rtl8139_disable() {
	rtl8139_outd(REG_RXCONF, 1 << 7); // disable receivinigngikng
	rtl8139_outw(REG_INTMASK, 0);
}

void rtl8139_reset(pci_t * pci) {
	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, rtl8139_callback);

	rtl8139_base = pci_find_iobase(pci);

	rtl8139_outb(REG_CONF2, 0);
	rtl8139_outb(REG_CMD, CMD_RESET);
	uint64_t target = ticks + pit_freq;
	while (target < ticks) {
		if ((rtl8139_inb(REG_CMD) & CMD_RESET) == 0) {
			break;
		}
	}

	rtl8139_mode = rtl8139_decode_mode(MODE_MANAGED);
	rtl8139_outd(REG_RXADDR, rtl8139_rx_base);
	rtl8139_outw(REG_INTMASK, INTMASK_RX | INTMASK_TX);
	rtl8139_outd(REG_RXCONF, rtl8139_mode | (1 << 7));
	rtl8139_outb(REG_CMD, CMD_ENABLE_RX | CMD_ENABLE_TX);
	memset((void *) rtl8139_rx_base, 0, 0x7fff);
}

void rtl8139_send(interface_t * interface, void * packet, size_t size) {
	if (interface->mode == MODE_OFF || interface->working == 0) {
		return;
	}
	rtl8139_send_packet(packet, size);
}

uint8_t rtl8139_decode_mode(int mode) {
	switch (mode) {
		default:
			return rtl8139_mode;
		case MODE_OFF:
			return 0;
		case MODE_MONITOR:
			return 0x0f; // all
		case MODE_MANAGED:
			return RXCONF_ACCEPT_BROADCAST | RXCONF_ACCEPT_PHYSICAL;
	}
}

int rtl8139_setmode(int mode) {
	uint8_t nibble = rtl8139_decode_mode(mode);
	rtl8139_mode = nibble;
	rtl8139_outd(REG_RXCONF, rtl8139_mode | (1 << 7));
	return 0;
}

int rtl8139_ctrl(interface_t * interface, int cmd, uint32_t op) {
	if (cmd == NET_CMD_ENABLE) {
		rtl8139_enable();
		interface->working = 1;
		return 0;
	}
	if (interface->mode == MODE_OFF || interface->working == 0) {
		return -1;
	}
	uint64_t mac;
	switch (cmd) {
		case NET_CMD_SET_MODE:
			return rtl8139_setmode(op);
		case NET_CMD_SET_MAC_LOW:
			mac = interface->mac & 0xffffffff00000000;
			mac |= (uint64_t) op;
			return rtl8139_setmac(mac);
		case NET_CMD_SET_MAC_HIGH:
			mac = interface->mac & 0x00000000ffffffff;
			mac |= (uint64_t) op << 32;
			return rtl8139_setmac(mac);
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
			rtl8139_disable();
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

uint64_t rtl8139_read_mac() {
	uint64_t mac = rtl8139_ind(REG_MAC);
	mac |= ((uint64_t) rtl8139_inw(REG_MAC + 4)) << 32;
	return mac;
}

int rtl8139_setmac(uint64_t mac) {
	rtl8139_outd(REG_MAC, mac & 0xffffffff);
	rtl8139_outw(REG_MAC + 4, (mac >> 32) & 0xffff);
	return 0;
}

int rtl8139_check(pci_t * pci) {
	if (pci->vendor == 0x10ec && pci->device == 0x8129) {
		return 1; // RTL8129 (even cheaper RTL8139)
	}
	if (pci->vendor == 0x10ec && pci->device == 0x8139) {
		return 1; // RTL8139
	}
	if (pci->vendor == 0x10ec && pci->device == 0x8138) {
		return 1; // RTL8139B
	}
	return 0;
}


int rtl8139_dev_init(pci_t * pci) {
	rtl8139_found++;
	if (rtl8139_found != 1) {
		return 0;
	}
	
	pci_cmd_set_flags(pci, PCI_CMD_MASTER); // enable dma (master mode)
	rtl8139_reset(pci);

	rtl8139_interface = net_create_interface(u"rtl8139", NULL, rtl8139_send, rtl8139_ctrl, IF_ETHER, 0);
	rtl8139_interface->working = 1;
	rtl8139_interface->mode = MODE_MANAGED;
	rtl8139_interface->speed = 10;
	rtl8139_interface->mac = rtl8139_read_mac();
	rtl8139_setmac(rtl8139_interface->mac);
	net_register_interface(rtl8139_interface);
	rtl8139_working = 1;
}

void rtl8139_init() {
	if (!network_enabled) {
		return;
	}
	pci_add_handler(rtl8139_dev_init, rtl8139_check);
}
