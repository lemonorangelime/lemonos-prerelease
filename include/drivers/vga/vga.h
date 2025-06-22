#pragma once

#include <stdint.h>

enum {
	VGA_REGISTER  = 0x3c0,
	VGA_DATA      = 0x3c0,
	VGA_ATTRIBUTE = 0x3c1,
	VGA_OUTPUT    = 0x3c2,
	VGA_STATUS    = 0x3c2,
	VGA_CRTC_REG  = 0x3d4,
	VGA_CRTC_DATA = 0x3d5,
	VGA_FLIPFLOP  = 0x3da,
};

void vga_crtc_outb(uint8_t reg, uint8_t byte);
void vga_crtc_outw(uint8_t reg, uint16_t word);
uint8_t vga_crtc_inb(uint8_t reg);
uint16_t vga_crtc_inw(uint8_t reg);
void vga_init();