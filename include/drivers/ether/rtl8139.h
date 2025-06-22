#pragma once

#include <net/net.h>
#include <bus/pci.h>

enum {
	REG_MAC,
	REG_FILTER = 0x08,
	REG_TXSTAT = 0x10,
	REG_TXADDR = 0x20,
	REG_RXADDR = 0x30,
	REG_CMD = 0x37,
	REG_RXTOP = 0x38,
	REG_INTMASK = 0x3c,
	REG_INTSTAT = 0x3e,
	REG_RXCONF = 0x44,
	REG_CONF1 = 0x51,
	REG_CONF2 = 0x52,
	REG_MODE_CTRL = 0x63,
};

enum {
	CMD_EMPTYRX   = 0b00001,
	CMD_ENABLE_TX = 0b00100,
	CMD_ENABLE_RX = 0b01000,
	CMD_RESET     = 0b10000,
};

enum {
	INTMASK_RX           = 0b000000001,
	INTMASK_RX_ERROR     = 0b000000010,
	INTMASK_TX           = 0b000000100,
	INTMASK_TX_ERROR     = 0b000001000,
	INTMASK_RX_OVERFLOW  = 0b000010000,
	INTMASK_RX_UNDERFLOW = 0b000100000,
	INTMASK_RX_OUT       = 0b001000000,
	INTMASK_TIMEOUT      = 0b010000000,
	INTMASK_ERROR        = 0b100000000,
};

enum {
	RXCONF_ACCEPT_BROADCAST = 0b1000,
	RXCONF_ACCEPT_MULTICAST = 0b0100,
	RXCONF_ACCEPT_PHYSICAL  = 0b0010,
	RXCONF_MONITOR_MODE     = 0b0001,
};

enum {
	RX_PACKET_OK        = 0b0000000000000001,
	RX_PACKET_ALIGNMENT = 0b0000000000000010,
	RX_PACKET_CRC_ERR   = 0b0000000000000100,
	RX_PACKET_LONG      = 0b0000000000001000,
	RX_PACKET_SHORT     = 0b0000000000010000,
	RX_PACKET_SYMBOL    = 0b0000000000100000,
	RX_PACKET_BROADCAST = 0b0010000000000000,
	RX_PACKET_PHYSICAL  = 0b0100000000000000,
	RX_PACKET_MULTICAST = 0b1000000000000000,
};

enum {
	MODE_CTRL_DUPLEX    = 0b000010000000,
	MODE_CTRL_NEGOTIATE = 0b100000000000,
};

#define RTL8139_DMA 0x18000

void rtl8139_send_packet(void * packet, size_t size);
void rtl8139_callback();
void rtl8139_enable();
void rtl8139_disable();
void rtl8139_reset(pci_t * pci);
int rtl8139_check(pci_t * pci);
void rtl8139_send(interface_t * interface, void * packet, size_t size);
uint8_t rtl8139_decode_mode(int mode);
int rtl8139_setmode(int mode);
int rtl8139_ctrl(interface_t * interface, int cmd, uint32_t op);
int rtl8139_setmac(uint64_t mac);
int rtl8139_dev_init(pci_t * pci);
void rtl8139_init();
