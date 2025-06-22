#include <bus/pci.h>
#include <interrupts/irq.h>
#include <input/input.h>
#include <multitasking.h>
#include <drivers/input/mouse.h>
#include <ports.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <drivers/vdebugger/vdebugger.h>
#include <unicode.h>
#include <string.h>
#include <stdio.h>

// dont look at this

static uint32_t vdebugger_iobase = 0;

void vdebugger_outb(uint8_t port, uint8_t data) {
	outb(vdebugger_iobase + port, data);
}

void vdebugger_outw(uint8_t port, uint16_t data) {
	outw(vdebugger_iobase + port, data);
}

void vdebugger_outd(uint8_t port, uint32_t data) {
	outd(vdebugger_iobase + port, data);
}

uint8_t vdebugger_inb(uint8_t port) {
	return inb(vdebugger_iobase + port);
}

uint16_t vdebugger_inw(uint8_t port) {
	return inw(vdebugger_iobase + port);
}

uint32_t vdebugger_ind(uint8_t port) {
	return ind(vdebugger_iobase + port);
}

void vdebugger_cmd(uint8_t cmd) {
	vdebugger_outb(VDEBUGGER_CMD, cmd);
}

void vdebugger_read(void * data, uint32_t size) {
	uint8_t * p = data;
	while (size--) {
		*p++ = vdebugger_inb(VDEBUGGER_OUT);
	}
}

void * vdebugger_read_data(uint32_t size) {
	if (size == 0) {
		return NULL;
	}
	uint8_t * data = malloc(size);
	vdebugger_read(data, size);
	return data;
}

void vdebugger_callback(registers_t * regs) {
	irq_ack(regs->int_no);

	uint16_t operation = vdebugger_inw(VDEBUGGER_OPERATION);
	uint32_t size = vdebugger_ind(VDEBUGGER_SIZE);
	uint8_t * data = vdebugger_read_data(size);
	switch (operation) {
		default:
			vdebugger_cmd(VDEBUGGER_CMD_REJECT);
			break;
		case VDEBUGGER_OPERATION_NOP:
			break;
		case VDEBUGGER_OPERATION_LOG:
			uint16_t * unicode = malloc((size * 2) + 4); // we use UTF-16 internally, so do this
			utf8toutf16l(data, unicode, size);

			stdout_state_t state = {ANSI_STATE_PRINTING, {0, 0, 0, 0, 0, 0, 0, 0}, 0, 0xffffffff};
			ansi_stdout_print(&state, unicode, ustrlen(unicode));
			free(unicode);
			break;
		case VDEBUGGER_OPERATION_INSERT_EVENT:
			break;
	}
	if (data) {
		free(data);
	}
	vdebugger_cmd(VDEBUGGER_CMD_ACK);
}

void vdebugger_reset(pci_t * pci) {
	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, vdebugger_callback);

	vdebugger_iobase = pci_find_iobase(pci);
	vdebugger_cmd(VDEBUGGER_CMD_START);
}

int vdebugger_found = 0;
int vdebugger_dev_init(pci_t * pci) {
	vdebugger_found++;
	if (vdebugger_found != 1) {
		cprintf(4, u"Ignoring %d%s Virtual Debugger device\n", vdebugger_found, number_text_suffix(vdebugger_found));
		return 0;
	}
	vdebugger_reset(pci);
}

int vdebugger_check(pci_t * pci) {
	if (pci->vendor == 0x1e34 && pci->device == 0x0001) {
		return 1; // Virtual Debugger
	}
	return 0;
}

void vdebugger_init() {
	pci_add_handler(vdebugger_dev_init, vdebugger_check);
}
