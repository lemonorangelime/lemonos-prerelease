#include <bus/pci.h>
#include <stdio.h>
#include <bus/usb.h>
#include <util.h>
#include <interrupts/irq.h>
#include <graphics/graphics.h>
#include <memory.h>
#include <string.h>

int usb_enabled = 1;

int usb_interface2type(int interface) {
	switch (interface) {
		case 0x00: return USB_CONTROLLER_UHCI;
		case 0x10: return USB_CONTROLLER_OHCI;
		case 0x20: return USB_CONTROLLER_EHCI;
		case 0x30: return USB_CONTROLLER_XHCI;
		case 0x80: return USB_CONTROLLER_UNSPECIFIED;
		case 0xfe: return USB_CONTROLLER_DEVICE;
	}
}

uint16_t * usb_type2name(int interface) {
	switch (interface) {
		case USB_CONTROLLER_UHCI: return u"UHCI";
		case USB_CONTROLLER_OHCI: return u"OHCI";
		case USB_CONTROLLER_EHCI: return u"EHCI";
		case USB_CONTROLLER_XHCI: return u"XHCI";
		case USB_CONTROLLER_UNSPECIFIED: return u"(Unspecified)";
		case USB_CONTROLLER_DEVICE: return u"(Device)";
	}
}



uint8_t usb_controller_cap_inb(usb_controller_t * controller, uint8_t address) {
	return *(uint8_t *) (controller->mmio + address);
}

uint16_t usb_controller_cap_inw(usb_controller_t * controller, uint8_t address) {
	return *(uint16_t *) (controller->mmio + address);
}

uint32_t usb_controller_cap_ind(usb_controller_t * controller, uint8_t address) {
	return *(uint32_t *) (controller->mmio + address);
}


void usb_controller_cap_outb(usb_controller_t * controller, uint8_t address, uint8_t data) {
	*(uint8_t *) (controller->mmio + address) = data;
}

void usb_controller_cap_outw(usb_controller_t * controller, uint8_t address, uint16_t data) {
	*(uint16_t *) (controller->mmio + address) = data;
}

void usb_controller_cap_outd(usb_controller_t * controller, uint8_t address, uint32_t data) {
	*(uint32_t *) (controller->mmio + address) = data;
}



uint8_t usb_controller_op_inb(usb_controller_t * controller, uint8_t address) {
	return *(uint8_t *) (controller->mmio + controller->caplen + address);
}

uint16_t usb_controller_op_inw(usb_controller_t * controller, uint8_t address) {
	return *(uint16_t *) (controller->mmio + controller->caplen + address);
}

uint32_t usb_controller_op_ind(usb_controller_t * controller, uint8_t address) {
	return *(uint32_t *) (controller->mmio + controller->caplen + address);
}


void usb_controller_op_outb(usb_controller_t * controller, uint8_t address, uint8_t data) {
	*(uint8_t *) (controller->mmio + controller->caplen + address) = data;
}

void usb_controller_op_outw(usb_controller_t * controller, uint8_t address, uint16_t data) {
	*(uint16_t *) (controller->mmio + controller->caplen + address) = data;
}

void usb_controller_op_outd(usb_controller_t * controller, uint8_t address, uint32_t data) {
	*(uint32_t *) (controller->mmio + controller->caplen + address) = data;
}



usb_controller_t * usb_create_controller(pci_t * pci) {
	usb_controller_t * controller = malloc(sizeof(usb_controller_t));
	controller->pci = pci;
	controller->type = usb_interface2type(pci->interface);
	controller->mmio = (void *) pci->bar0;
	controller->caplen = usb_controller_cap_inb(controller, EHCI_REG_CAPLEN);
	return controller;
}

int usb_controller_reset(usb_controller_t * controller) {
	usb_controller_op_outd(controller, EHCI_REG_CMD, 0);
	sleep_seconds(1);
	usb_controller_op_outd(controller, EHCI_REG_CMD, EHCI_CMD_RESET);

	int i = 3;
	while (i--) {
		uint32_t cmd = usb_controller_op_ind(controller, EHCI_REG_CMD);
		if ((cmd & EHCI_CMD_RESET) == 0) {
			return 0;
		}
	}
	return 1;
}

void usb_controller_callback(registers_t * regs) {
	cprintf(LEGACY_COLOUR_RED, u"USB: IRQ - Don't know what to do...\n");
}

int usb_dev_init(pci_t * pci) {
	int type = usb_interface2type(pci->interface);
	uint16_t * name = usb_type2name(type);
	if (type != USB_CONTROLLER_EHCI) {
		cprintf(LEGACY_COLOUR_RED, u"USB: ignoring unsupported controller\n");
		return 0;
	}
	printf(u"USB: found USB %s controller\n", name);
	pci_cmd_set_flags(pci, PCI_CMD_MASTER | PCI_CMD_MEM);

	usb_controller_t * controller = usb_create_controller(pci);
	uint8_t ports = usb_controller_cap_ind(controller, EHCI_REG_STRUCTURE) & EHCI_STRUCTURE_DOWNSTREAMS;
	printf(u"USB: %d ports\n", ports);
	return 0;

	usb_controller_op_outd(controller, EHCI_REG_IRQ, 0);
	if (usb_controller_reset(controller)) {
		cprintf(LEGACY_COLOUR_RED, u"USB: FATAL ERROR - Could not reset controller\n");
		return 0;
	}
	usb_controller_op_outd(controller, EHCI_REG_SEGMENT, 0);

	int irq = pci_get_irq(pci);
	irq_set_handler(32 + irq, usb_controller_callback);

	usb_controller_op_outd(controller, EHCI_REG_IRQ, EHCI_IRQ_ENABLE_TRANSFER | EHCI_IRQ_ENABLE_ERROR | EHCI_IRQ_ENABLE_SYSTEM_ERROR | EHCI_IRQ_ENABLE_ASYNC_WARNING);

	usb_controller_op_outd(controller, EHCI_REG_FRAME_BASE, USB_DMA); // todo: not this lmao what

	uint32_t itd_size = 16384;
	uint32_t sitd_size = 7680;
	uint32_t buffer_size = 32256;

	uint32_t base = USB_DMA + 4096;
	memset((void *) base, 0, 4096);
}

int usb_check(pci_t * pci) {
	if (pci->class == 0x0c && pci->subclass == 0x03) {
		return 1;
	}
	return 0;
}

void usb_init() {
	if (!usb_enabled) {
		return;
	}
	pci_add_handler(usb_dev_init, usb_check);
}
