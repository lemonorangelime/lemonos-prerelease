#include <drivers/audio/sb16.h>
#include <bus/pci.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <pit.h>
#include <util.h>
#include <interrupts/irq.h>
#include <ports.h>
#include <string.h>

// doesnt work or do anything

uint32_t sb16_base;
uint32_t sb16_dma = SB16_DMA;
int sb16_found = 0;
int sb16_type = 0;

uint8_t sb16_inb(uint16_t reg) {
	return inb(sb16_base + reg);
}

uint8_t sb16_outb(uint16_t reg, uint8_t data) {
	outb(sb16_base + reg, data);
}

void sb16_delay() {
	uint64_t target = ticks + 1;
	while (ticks < target) {}
}

void sb16_exec(uint8_t cmd) {
	sb16_outb(SB16_CMD, cmd);
	sb16_delay();
}

uint8_t sb16_cmd(uint8_t cmd, uint8_t data, int has_data) {
	sb16_exec(cmd);
	if (has_data) {
		sb16_exec(data);
	}
	return sb16_inb(SB16_READ);
}

uint8_t sb16_cmd_mixer(uint8_t cmd, uint8_t data, int has_data) {
	sb16_outb(SB16_MIXER_RAP, cmd);
	if (has_data) {
		sb16_outb(SB16_MIXER_DATA, data);
	}
}

void sb16_set_rate(uint16_t rate) {
	sb16_exec(SB16_CMD_SAMPLE_RATE);
	sb16_exec(rate >> 8);
	sb16_exec(rate & 0xff);
}

int sb16_reset_dsp() {
	sb16_outb(SB16_RESET, 1);
	sb16_delay();
	sb16_outb(SB16_RESET, 0);


	uint64_t target = ticks + (pit_freq / 100);
	uint8_t status = sb16_inb(SB16_STATUS);
	if (~status & 128) {
		return 0;
	}
	status = sb16_inb(SB16_READ);
	if (status != 0xaa) {
		return 0;
	}
	return 1;
}

void sb16_callback(registers_t * regs) {
	sb16_inb(SB16_STATUS);
	sb16_inb(SB16_IRQ_ACK);
}

void sb16_reset() {
	uint16_t size = (SB16_DMA_SIZE / 2) - 1;
	memset((void *) sb16_dma, 0, SB16_DMA_SIZE);
	if (!sb16_reset_dsp()) {
		return;
	}
	irq_set_handler(32 + 5, sb16_callback);

	sb16_cmd_mixer(SB16_MIXER_IRQ_LINE, 0x02, 1);

	outb(0xd4, 5);
	outb(0xd8, 0xff);
	outb(0xd6, 0x59);

	outb(0xc4, sb16_dma & 0xff);
	outb(0xc4, (sb16_dma >> 8) & 0xff);
	outb(0x8b, (sb16_dma >> 16) & 0xff);

	outb(0xc6, SB16_DMA_SIZE & 0xff);
	outb(0xc6, (SB16_DMA_SIZE >> 8) & 0xff);

	outb(0xd4, 1);

	sb16_set_rate(44100);

	sb16_outb(SB16_WRITE, 0xb6);
	sb16_outb(SB16_WRITE, 0x10);
	sb16_outb(SB16_WRITE, size & 0xff);
	sb16_outb(SB16_WRITE, (size >> 8) & 0xff);


	sb16_outb(SB16_WRITE, SB16_CMD_ON);
	sb16_outb(SB16_WRITE, SB16_CMD_RESUME_CHANNEL16);
}

int sb16_check(pci_t * pci) {
	if (0) {
		return 1; // SB16 PCI
	}
	return 0;
}

int sb16_detect(uint32_t base) {
	sb16_base = base; // evil global

	uint8_t test = sb16_cmd(SB16_CMD_GET_VERSION, 0, 0);
	if (test < 1 || test > 4) {
		return 0; // not a sound blaster :c
	}

	sb16_reset_dsp();
	sleep_seconds(1);
	test = sb16_inb(SB16_READ);
	return test == 0xaa; // is sound blaster?
}

int sb16_unsafe_scan() {
	uint16_t ports[] = {0x220, 0x240, 0x260, 0x280}; // jumper addresses
	size_t length = sizeof(ports) / sizeof(ports[0]);
	for (int i = 0; i < length; i++) {
		uint16_t port = ports[i];
		if (sb16_detect(port)) {
			return 1;
		}
	}
	return 0;
}

int sb16_dev_init(pci_t * pci) {
	sb16_found++;
	if (sb16_found != 1) {
		return 0;
	}

	pci_cmd_set_flags(pci, PCI_CMD_MASTER | PCI_CMD_MEM); // enable dma (master mode)
	sb16_base = pci_find_iobase(pci);
	sb16_reset();
}

void sb16_init() {
	pci_add_handler(sb16_dev_init, sb16_check); // try pci first

	if (sb16_found == 0) {
		if (sb16_unsafe_scan()) {
			sb16_found = 1;
			sb16_reset();
		}
	}
}
