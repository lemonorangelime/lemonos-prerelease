#include <drivers/misc/vbox.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <ports.h>
#include <string.h>
#include <drivers/input/mouse.h>
#include <memory.h>
#include <graphics/graphics.h>
#include <bus/pci.h>
#include <interrupts/irq.h>
#include <graphics/displays.h>

static uint16_t vbox_io = 0;
static vbox_vmm_t * vbox_vmm = 0;
static void * vbox_packet = (void *) 0x6e000;

void * vbox_init_packet(uint32_t type) {
	vbox_header_t * header = vbox_packet;
	header->type = type;
	header->response = 0;
	switch (type) {
		case VBOX_GET_MOUSE:
		case VBOX_SET_MOUSE:
			header->size = sizeof(vbox_mouse_packet_t);
			break;
		case VBOX_ACK:
			header->size = sizeof(vbox_ack_packet_t);
			break;
		case VBOX_REPORT_INFO:
			header->size = sizeof(vbox_guest_info_packet_t);
			break;
		case VBOX_GET_DISPLAY:
			header->size = sizeof(vbox_display_update_packet_t);
			break;
		case VBOX_REPORT_CAPABILITIES:
			header->size = sizeof(vbox_guest_caps_packet_t);
			break;
	}
	memset(vbox_packet + sizeof(vbox_header_t), 0, header->size - sizeof(vbox_header_t));
	return vbox_packet;
}

void vbox_transfer_packet(void * packet) {
	outd(vbox_io, (uint32_t) packet);
}

void vbox_set_mouse(uint32_t flags) {
	vbox_mouse_packet_t * mouse = vbox_init_packet(VBOX_SET_MOUSE);
	mouse->flags = flags;
	vbox_transfer_packet(mouse);
}

void vbox_set_caps(uint32_t caps) {
	vbox_guest_caps_packet_t * gcaps = vbox_init_packet(VBOX_REPORT_CAPABILITIES);
	gcaps->caps = caps;
	vbox_transfer_packet(gcaps);
}

void vbox_set_guest_info(uint32_t ostype) {
	vbox_guest_info_packet_t * ginfo = vbox_init_packet(VBOX_REPORT_INFO);
	ginfo->version = VBOX_VERSION;
	ginfo->ostype = ostype;
	vbox_transfer_packet(ginfo);
}

void vbox_ack_irq(uint32_t events) {
	vbox_ack_packet_t * ack = vbox_init_packet(VBOX_ACK);
	ack->events = events;
	vbox_transfer_packet(ack);
}

vbox_display_update_packet_t * vbox_get_display() {
	vbox_display_update_packet_t * display = vbox_init_packet(VBOX_GET_DISPLAY);
	display->ack = 1;
	vbox_transfer_packet(display); // call twice, not a typo
	vbox_transfer_packet(display);
	return display;
}

vbox_mouse_packet_t * vbox_get_mouse() {
	vbox_mouse_packet_t * mouse = vbox_init_packet(VBOX_GET_MOUSE);
	vbox_transfer_packet(mouse);
	return mouse;
}

void vbox_display_update() {
	vbox_display_update_packet_t * vboxdisplay = vbox_get_display();
	display_t * display = display_get_default();
	display_resize(display, vboxdisplay->width, vboxdisplay->height);
}

void vbox_mouse_update() {
	vbox_mouse_packet_t * mouse = vbox_get_mouse();

	int32_t old_mouse_x = mouse_x;
	int32_t old_mouse_y = mouse_y;
	mouse_x = (mouse->x * root_window.size.width) / 0xffff;
	mouse_y = (mouse->y * root_window.size.height) / 0xffff;
	clip_mouse(&mouse_x, &mouse_y);

	cursor.x = mouse_x;
	cursor.y = mouse_y;

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

void vbox_callback(registers_t * regs) {
	if (!vbox_vmm->irq_events) {
		return;
	}

	uint32_t events = vbox_vmm->irq_events;
	vbox_ack_irq(events);

	if (events & VBOX_EVENT_MOUSE) {
		vbox_mouse_update();
	}
	if (events & VBOX_EVENT_DISPLAY) {
		vbox_display_update();
	}
}

void vbox_reset(pci_t * pci) {
	vbox_io = pci->bar0 & 0xfffffffc;
	vbox_vmm = (vbox_vmm_t *) (pci->bar1 & 0xfffffff0);

	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, vbox_callback);

	vbox_header_t * header = vbox_packet;
	header->version = VBOX_HEADER_VERSION;
	header->padding = 0;
	header->padding1 = 0;

	vbox_set_guest_info(VBOX_OSTYPE_UNKNOWN);
	vbox_set_caps(VBOX_CAPS_GRAPHICS);
	vbox_set_mouse(VBOX_MOUSE_ENABLE);
	vbox_get_display();

	vbox_vmm->irq_mask = 0xffffffff;
}

int vbox_check(pci_t * pci) {
	return (pci->vendor == 0x80ee && pci->device == 0xcafe);
}

static int vbox_found = 0;
int vbox_dev_init(pci_t * pci) {
	vbox_found++;
	if (vbox_found != 1) {
		cprintf(4, u"Ignoring %d%s VBox Guest Interface\n", vbox_found, number_text_suffix(vbox_found));
		return 0;
	}
	vbox_reset(pci);
}

void vbox_init() {
	pci_add_handler(vbox_dev_init, vbox_check);
}
