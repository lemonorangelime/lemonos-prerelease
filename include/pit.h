#pragma once

#include <asm.h>
#include <stdint.h>

extern volatile uint64_t ticks;
extern uint32_t pit_freq;
extern int pit_enabled;

enum {
	PIT_CHANNEL0         = 0b00000000,
	PIT_CHANNEL1         = 0b01000000,
	PIT_CHANNEL2         = 0b10000000,
	PIT_CHANNELLATCH     = 0b11000000,
	PIT_ACCESS_LATCH     = 0b00000000,
	PIT_ACCESS_LOW       = 0b00010000,
	PIT_ACCESS_HIGH      = 0b00100000,
	PIT_ACCESS_LOHI      = 0b00110000,
	PIT_MODE_COUNT       = 0b00000000,
	PIT_MODE_ONESHOT     = 0b00000010,
	PIT_MODE_RATE        = 0b00000100,
	PIT_MODE_SQUAREWAVE  = 0b00000110,
	PIT_MODE_SOFTSTROBE  = 0b00001000,
	PIT_MODE_HARDSTROBE  = 0b00001010,
	PIT_MODE_RATE2       = 0b00001100,
	PIT_MODE_SQUAREWAVE2 = 0b00001110,
	PIT_BCD_BINARY       = 0b00000000,
	PIT_BCD_BCD          = 0b00000001,
};

void pit_callback(registers_t * regs);
void pit_disable();
void pit_init(uint32_t freq);