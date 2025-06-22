#include <bus/pci.h>
#include <interrupts/irq.h>
#include <input/input.h>
#include <multitasking.h>
#include <drivers/input/mouse.h>
#include <ports.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <drivers/input/vmouse.h>

//; dont look at thsi

static uint32_t vmouse_iobase = 0;

void vmouse_outb(uint8_t port, uint8_t data) {
	outb(vmouse_iobase + port, data);
}

uint32_t vmouse_ind(uint8_t port) {
	return ind(vmouse_iobase + port);
}

void vmouse_cmd(uint8_t cmd) {
	vmouse_outb(VMOUSE_CMD, cmd);
}

void vmouse_callback(registers_t * regs) {
	int32_t old_mouse_x = mouse_x;
	int32_t old_mouse_y = mouse_y;
	uint32_t buttons = vmouse_ind(VMOUSE_BUTTONS);
	int32_t mouse_z = vmouse_ind(VMOUSE_Z_AXIS);
	mouse_x = vmouse_ind(VMOUSE_X_AXIS);
	mouse_y = vmouse_ind(VMOUSE_Y_AXIS);
	vmouse_cmd(VMOUSE_CMD_ACK);
	clip_mouse(&mouse_x, &mouse_y);
	mouse_held.left = (buttons & VMOUSE_BUTTON_LEFT) != 0;
	mouse_held.right = (buttons & VMOUSE_BUTTON_RIGHT) != 0;
	mouse_held.middle = (buttons & VMOUSE_BUTTON_MIDDLE) != 0;

	mouse_event_t * event = malloc(sizeof(mouse_event_t));
	event->type = EVENT_MOUSE;
	event->x = mouse_x;
	event->y = mouse_y;
	event->delta_x = mouse_x - old_mouse_x;
	event->delta_y = -(mouse_y - old_mouse_y);
	event->bdelta_x = mouse_x - old_mouse_x;
	event->bdelta_y = -(mouse_y - old_mouse_y);
	event->held = &mouse_held;
	broadcast_event((event_t *) event);
}

void vmouse_reset(pci_t * pci) {
	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, vmouse_callback);

	vmouse_iobase = pci_find_iobase(pci);
	vmouse_cmd(VMOUSE_CMD_START);
	mouse_disable();
	disable_cursor_icon();
}

int vmouse_found = 0;
int vmouse_dev_init(pci_t * pci) {
	vmouse_found++;
	if (vmouse_found != 1) {
		cprintf(4, u"Ignoring %d%s Virtual Mouse device\n", vmouse_found, number_text_suffix(vmouse_found));
		return 0;
	}
	vmouse_reset(pci);
}

int vmouse_check(pci_t * pci) {
	if (pci->vendor == 0x1e34 && pci->device == 0x0000) {
		return 1; // Virtual Mouse
	}
	return 0;
}

void vmouse_init() {
	pci_add_handler(vmouse_dev_init, vmouse_check);
}
