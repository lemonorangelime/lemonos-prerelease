#include <stdint.h>

void outb(uint16_t port, uint8_t val) {
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port) :"memory");
}

void outw(uint16_t port, uint16_t val) {
	asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port) :"memory");
}

void outd(uint16_t port, uint32_t val) {
	asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port) :"memory");
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}

uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}

uint32_t ind(uint16_t port) {
	uint32_t ret;
	asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}

// who tf uses this inX_bin() shit in LemonOS v2

void inb_bin(uint16_t port, uint8_t * buffer, size_t length) {
	size_t i = 0;
	while (length--) {
		buffer[i] = inb(port);
		i++;
	}
}

void inw_bin(uint16_t port, uint16_t * buffer, size_t length) {
	size_t i = 0;
	while (length--) {
		buffer[i] = inw(port);
		i++;
	}
}

void inl_bin(uint16_t port, long * buffer, size_t length) {
	size_t i = 0;
	while (length--) {
		buffer[i] = ind(port);
		i++;
	}
}

void ind_bin(uint16_t port, uint32_t * buffer, size_t length) {
	size_t i = 0;
	while (length--) {
		buffer[i] = ind(port);
		i++;
	}
}
