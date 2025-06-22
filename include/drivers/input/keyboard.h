#pragma once

enum {
	PS2_DATA = 0x60,
	PS2_STATUS = 0x64, // same
	PS2_WRITE = 0x64,
};

enum {
	PS2_STATUS_OUT_BUSY    = 0b00000001,
	PS2_STATUS_IN_BUSY     = 0b00000010,
	PS2_STATUS_POSTED      = 0b00000100,
	PS2_STATUS_COMMAND     = 0b00001000,
	PS2_STATUS_LOCKED      = 0b00010000,
	PS2_STATUS_TIMEOUT     = 0b00100000,
	PS2_STATUS_TIMEOUT_ERR = 0b01000000,
	PS2_STATUS_CRC_ERR     = 0b10000000,
};

enum {
	PS2_COMMAND_READ_EEPROM           = 0x20, // or me with address (0x20 | 0x10 == read address 0x10)
	PS2_COMMAND_WRITE_EEPROM          = 0x60, // or me with address (0x60 | 0x10 == read address 0x10)
	PS2_COMMAND_DISABLE_MOUSE         = 0xa7,
	PS2_COMMAND_ENABLE_MOUSE          = 0xa8,
	PS2_COMMAND_CONTROLLER_POST       = 0xaa,
	PS2_COMMAND_DEVICE_POST           = 0xab,
	PS2_COMMAND_DISABLE_KEYBOARD      = 0xad,
	PS2_COMMAND_ENABLE_KEYBOARD       = 0xae,
	PS2_COMMAND_READ_OUTPUT           = 0xd0,
	PS2_COMMAND_WRITE_OUTPUT          = 0xd1,
	PS2_COMMAND_WRITE_KEYBOARD_OUTPUT = 0xd2,
	PS2_COMMAND_WRITE_MOUSE_OUTPUT    = 0xd3,
	PS2_COMMAND_WRITE_MOUSE_INPUT     = 0xd4,
	PS2_COMMAND_STROBE_PIN            = 0xf0,
};

enum {
	PS2_CONFIG_KEYBOARD_IRQ   = 0b00000001,
	PS2_CONFIG_MOUSE_IRQ      = 0b00000010,
	PS2_CONFIG_POSTED         = 0b00000100,
	PS2_CONFIG_KEYBOARD_CLOCK = 0b00010000,
	PS2_CONFIG_MOUSE_CLOCK    = 0b00100000,
	PS2_CONFIG_KEYBOARD_TRANS = 0b01000000,
};

enum {
	PS2_MOUSE_IDENTIFY       = 0xf2,
	PS2_MOUSE_SET_SAMPLERATE = 0xf3,
	PS2_MOUSE_ENABLE_IRQ     = 0xf4,
	PS2_MOUSE_DISABLE_IRQ    = 0xf5,
	PS2_MOUSE_RESET          = 0xf6,
};

enum {
	PS2_KEYBOARD_LED         = 0xed,
	PS2_KEYBOARD_IDENTIFY    = 0xf2,
	PS2_KEYBOARD_SET_REPEAT  = 0xf3,
	PS2_KEYBOARD_ENABLE_IRQ  = 0xf4,
	PS2_KEYBOARD_DISABLE_IRQ = 0xf5,
	PS2_KEYBOARD_RESET       = 0xf6,
};

enum {
	KEYBOARD_OUTPUT = 0,
	KEYBOARD_INPUT = 1,
};

extern int keyboard_mouse;

void keyboard_init();
uint8_t keyboard_read();
uint8_t keyboard_read_ram(uint8_t addr);
void keyboard_write_ram(uint8_t addr, uint8_t data);
void keyboard_poll(uint8_t is_in);