#include <asm.h>
#include <stdint.h>
#include <stdio.h>
#include <interrupts/irq.h>
#include <ports.h>
#include <pit.h>
#include <util.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <input/input.h>
#include <multitasking.h>
#include <power/sleep.h>

volatile uint64_t ticks = 0;
uint32_t pit_freq;
int pit_enabled = 0;

// fix me !!
void pit_callback(registers_t * regs) {
	outb(0x20, 0x20); // ack the PIC
	if (in_sleep_mode) {
		return; // skip (bad ?)
	}
	if (sleep_mode_schedule != 0) {
		if (ticks > sleep_mode_schedule) {
			sleep_enter();
			return; // this tick is skipped (bad ?)
		}
	}
	ticks++;
	switch_task();
}

void pit_disable() {
	outb(0x43, PIT_CHANNEL0 | PIT_ACCESS_LOHI);
	outb(0x40, 0);
	outb(0x40, 0);
	outb(0x43, 0);
	pit_enabled = 0;
}

void pit_init(uint32_t freq) {
	uint32_t divisor = (uint32_t) (1193181 / freq);
	pit_freq = freq;
	pit_enabled = 1;
	irq_set_handler(32, pit_callback);
	outb(0x43, PIT_CHANNEL0 | PIT_ACCESS_LOHI | PIT_MODE_SQUAREWAVE);
	outb(0x40, divisor & 0xff);
	outb(0x40, (divisor >> 8) & 0xff);
}
