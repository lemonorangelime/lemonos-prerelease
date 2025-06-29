#pragma once

enum {
	VMOUSE_X_AXIS = 0x00,
	VMOUSE_Y_AXIS = 0x04,
	VMOUSE_Z_AXIS = 0x08,
	VMOUSE_BUTTONS = 0x0c,
	VMOUSE_CMD = 0x10,
};

enum {
	VMOUSE_CMD_START = 0x01,
	VMOUSE_CMD_ACK   = 0x02,
};

enum {
	VMOUSE_BUTTON_LEFT   = 0b0001,
	VMOUSE_BUTTON_RIGHT  = 0b0010,
	VMOUSE_BUTTON_MIDDLE = 0b0100,
};

void vmouse_init();