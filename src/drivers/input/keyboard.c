#include <interrupts/irq.h>
#include <drivers/input/keyboard.h>
#include <stdint.h>
#include <ports.h>
#include <stdio.h>
#include <input/input.h>
#include <memory.h>
#include <graphics/graphics.h>
#include <drivers/input/mouse.h>
#include <power/sleep.h>

int keyboard_mouse = 0;
int next_byte = 0;
static keyboard_held_t held;

void keyboard_poll(uint8_t is_in) {
	uint32_t timeout = 100000;
	uint8_t bits = PS2_STATUS_OUT_BUSY;
	if (is_in) {
		bits = PS2_STATUS_IN_BUSY;
	}
	while (timeout--) {
			if (!(inb(PS2_STATUS) & bits)) {
					return;
			}
	}
}

void keyboard_callback(registers_t * regs) {
	keyboard_poll(KEYBOARD_INPUT);
	uint32_t keycode = inb(PS2_DATA);
	int pressed = 1;
	kbd_event_t * event;

	in_sleep_mode = 0; // wake from sleep mode if it was on

	if (keycode == 224) {
		next_byte = 1; // this is multi byte sequence
		return;
	}

	if (keycode > 128) {
		keycode -= 128;
		pressed = 0;
	}

	if (next_byte) {
		next_byte = 0;
		switch (keycode) {
			default:
				//if (gfx_init_done) {
				//	cprintf(LEGACY_COLOUR_RED, u"Unknown key 0x%x\n", keycode);
				//}
				return;
			case 0x4f:
				keycode = 0; // silence end key (used by 86box)
				break;
			case 0x38:
				keycode = 0x54; // ralt replacement
				break;
			case 0x1d:
				keycode = 0x55; // rctrl replacement
				break;
			case 0x2a:
				keycode = 0x56; // sysrq replacement
				break;
			case 0x5b:
			case 0x5c:
				keycode = 0x57; // super replacement
				break;
			case 0x48:
				keycode = 0x5a; // arrow up replacement
				break;
			case 0x4d:
				keycode = 0x5b; // arrow right replacement
				break;
			case 0x4b:
				keycode = 0x5c; // arrow left replacement
				break;
			case 0x50:
				keycode = 0x5d; // arrow down replacement
				break;
			case 0x37:
				return; // duplicate
		}
	}

	switch (keycode) {
		case 0x2a:
			held.lshift = pressed;
			break;
		case 0x36:
			held.rshift = pressed;
			break;
		case 0x38:
			held.lalt = pressed;
			break;
		case 0x54:
			held.ralt = pressed;
			held.meta = pressed;
			break;
		case 0x1d:
			held.lctrl = pressed;
			break;
		case 0x55:
			held.rctrl = pressed;
			break;
		case 0x56:
			held.sysrq = pressed;
			break;
		case 0x57:
			held.super = pressed;
			break;
	}

	if (pressed) {
		if (keycode == 0x3A) {
			held.caps = !held.caps;
		}
	}

	event = malloc(sizeof(kbd_event_t) + 8);
	event->type = EVENT_KEYBOARD;
	event->keycode = keycode;
	event->pressed = pressed;
	event->held = &held;
	broadcast_event((event_t *) event);
}

uint8_t keyboard_read() {
	keyboard_poll(KEYBOARD_OUTPUT);
	uint8_t stat = inb(PS2_DATA);
	return stat;
}

uint8_t keyboard_read_ram(uint8_t addr) {
	outb(PS2_WRITE, PS2_COMMAND_READ_EEPROM | addr);
	return keyboard_read();
}

void keyboard_write_ram(uint8_t addr, uint8_t data) {
	outb(PS2_WRITE, PS2_COMMAND_WRITE_EEPROM | addr);
	keyboard_poll(KEYBOARD_INPUT);
	outb(PS2_DATA, data);
}

void keyboard_init() {
	outb(PS2_WRITE, PS2_COMMAND_DISABLE_KEYBOARD); // self explainitory
	keyboard_poll(KEYBOARD_INPUT);
	outb(PS2_WRITE, PS2_COMMAND_DISABLE_MOUSE); // self explainitory
	keyboard_poll(KEYBOARD_INPUT);

	// this is here now to prevent spurious interrupts during the time between interrupts are enabled and we are called
	irq_set_handler(33, keyboard_callback);
	outb(PS2_WRITE, PS2_COMMAND_ENABLE_KEYBOARD); // self explainitory
	keyboard_poll(KEYBOARD_INPUT);

	// enable keyboard IRQ
	uint8_t status = keyboard_read_ram(0);
	keyboard_poll(KEYBOARD_INPUT);
	keyboard_write_ram(0, status | PS2_CONFIG_KEYBOARD_IRQ);
}
