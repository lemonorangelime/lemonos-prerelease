#include <power/acpi/acpi.h>
#include <ports.h>
#include <util.h>
#include <panic.h>
#include <pit.h>
#include <multitasking.h>
#include <stdint.h>
#include <drivers/input/keyboard.h>

void reboot() {
	if (acpi_working && acpi_reboot()) {
		return;
	}
	while (inb(PS2_STATUS) & PS2_STATUS_IN_BUSY) {}
	outb(PS2_WRITE, PS2_COMMAND_STROBE_PIN | 0x0e); // strobe CPU reset pin
	handle_error(SHUTDOWN_FAILURE, 0);
	halt();
}

void shutdown() {
	if (acpi_working && acpi_shutdown()) {
		return;
	}
	handle_error(SHUTDOWN_FAILURE, 0);
	halt();
}

int interrupts_enabled() {
	uint32_t eflags;
	asm volatile ("pushf; pop %0" : "=g"(eflags));
	return (eflags >> 9) & 1;
}

void interrupts_restore(int flag) {
	if (!flag) {
		disable_interrupts();
		return;
	}
	enable_interrupts();
}

void sleep_ticks(int time) {
	uint64_t target = ticks + time;
	while (target > ticks) {
		yield();
	}
}

void sleep_seconds(int time) {
	uint64_t target = ticks + (time * pit_freq);
	while (target > ticks) {
		yield();
	}
}

#define do_swap(type, x, y) type temp = *x;\
	*x = *y; \
	*y = temp;

#define create_swap_function(type, name) void name(type * x, type * y) { \
		do_swap(type, x, y); \
	}

create_swap_function(uint8_t, swap_p8);
create_swap_function(uint16_t, swap_p16);
create_swap_function(uint32_t, swap_p32);
create_swap_function(uint64_t, swap_p64);

uint16_t htonw(uint16_t d) {
	return (d << 8) | (d >> 8);
}

uint32_t htond(uint32_t d) {
	return ((d >> 24) & 0xff) | ((d << 8) & 0xff0000) | ((d >> 8) & 0xff00) | ((d << 24) & 0xff000000);
}

uint64_t htonq(uint64_t d) {
	d = (d & 0x00000000FFFFFFFF) << 32 | (d & 0xFFFFFFFF00000000) >> 32;
	d = (d & 0x0000FFFF0000FFFF) << 16 | (d & 0xFFFF0000FFFF0000) >> 16;
	d = (d & 0x00FF00FF00FF00FF) << 8 | (d & 0xFF00FF00FF00FF00) >> 8;
	return d;
}

uint16_t ntohw(uint16_t d) {
	return htonw(d);
}

uint32_t ntohd(uint32_t d) {
	return htond(d);
}

uint64_t ntohq(uint64_t d) {
	return htonq(d);
}
