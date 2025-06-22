#include <drivers/vga/vga.h>
#include <graphics/displays.h>
#include <ports.h>

void vga_crtc_outb(uint8_t reg, uint8_t byte) {
	outb(VGA_CRTC_REG, reg);
	outb(VGA_CRTC_DATA, byte);
}

void vga_crtc_outw(uint8_t reg, uint16_t word) {
	outb(VGA_CRTC_REG, reg);
	outb(VGA_CRTC_DATA, word);
}

uint8_t vga_crtc_inb(uint8_t reg) {
	outb(VGA_CRTC_REG, reg);
	return inb(VGA_CRTC_DATA);
}

uint16_t vga_crtc_inw(uint8_t reg) {
	outb(VGA_CRTC_REG, reg);
	return inb(VGA_CRTC_DATA);
}

void vga_init() {

}