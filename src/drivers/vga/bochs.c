#include <drivers/vga/bochs.h>
#include <graphics/displays.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <ports.h>
#include <math.h>
#include <bus/pci.h>

void bochsvga_write(uint16_t reg, uint16_t data) {
    outw(0x01ce, reg);
    outw(0x01cf, data);
}

uint16_t bochsvga_read(uint16_t reg) {
    outw(0x01ce, reg);
    return inw(0x01cf);
}

uint16_t bochsvga_bpp_convert(int bpp) {
	switch (bpp) {
		case 32:
			return BOCHSVGA_BPP32;
		case 24:
			return BOCHSVGA_BPP24;
		case 16:
			return BOCHSVGA_BPP16;
		case 15:
			return BOCHSVGA_BPP15;
		case 8:
			return BOCHSVGA_BPP8;
		case 4:
			return BOCHSVGA_BPP4;
	}
}


void bochsvga_resize(display_t * display, int width, int height) {
	width = rgb_align(width, display->bpp);
	height = rgb_align(height, display->bpp);
	bochsvga_write(BOCHSVGA_ENABLED, 0);
	bochsvga_write(BOCHSVGA_XRES, width);
	bochsvga_write(BOCHSVGA_YRES, height);
	bochsvga_write(BOCHSVGA_BPP, bochsvga_bpp_convert(display->bpp));
	bochsvga_write(BOCHSVGA_ENABLED, 1 | BOCHSVGA_FB_ENABLED | BOCHSVGA_NO_CLEAR);
	display->width = width;
	display->height = height;
}

void bochsvga_crunch(display_t * display, int bpp) {
	display->width = rgb_align(display->width, bpp);
	display->height = rgb_align(display->height, bpp);
	bochsvga_write(BOCHSVGA_ENABLED, 0);
	bochsvga_write(BOCHSVGA_XRES, display->width);
	bochsvga_write(BOCHSVGA_YRES, display->height);
	bochsvga_write(BOCHSVGA_BPP, bochsvga_bpp_convert(bpp));
	bochsvga_write(BOCHSVGA_ENABLED, 1 | BOCHSVGA_FB_ENABLED | BOCHSVGA_NO_CLEAR);
	display->bpp = bpp;
}

void bochsvga_reset(pci_t * pci) {
	display_t * display = display_create(DISPLAY_VGA, bochsvga_resize, bochsvga_crunch);
	display->bpp = root_window.bpp;
	display->fb = root_window.fb;
	display->height = root_window.size.height;
	display->width = root_window.size.width;
	display->pci = pci;
	display_register(display);
	display_listen(display, gfx_resize_callback, NULL);
}

static int bochsvga_found = 0;
int bochsvga_dev_init(pci_t * pci) {
	bochsvga_found++;
	if (bochsvga_found != 1) {
		cprintf(LEGACY_COLOUR_RED, u"Ignoring %d%s Bochs VGA device\n", bochsvga_found, number_text_suffix(bochsvga_found));
		return 0;
	}
	bochsvga_reset(pci);
}

int bochsvga_check(pci_t * pci) {
	if (pci->vendor == 0x15ad && pci->device == 0x0405) {
		return 1; // VMware SVGA II
	}
	if (pci->vendor == 0x80ee && pci->device == 0xbeef) {
		return 1; // Virtualbox VGA adapter
	}
	if (pci->vendor == 0x1234 && pci->device == 0x1111) {
		return 1; // Generic SVGA
	}
	return 0;
}

void bochsvga_init() {
	pci_add_handler(bochsvga_dev_init, bochsvga_check);
}
