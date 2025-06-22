#include <math.h>
#include <graphics/graphics.h>
#include <ports.h>
#include <asm.h>
#include <interrupts/irq.h>
#include <panic.h>
#include <drivers/input/keyboard.h>
#include <input/input.h>
#include <multitasking.h>
#include <multiboot.h>
#include <drivers/input/keyboard.h>
#include <memory.h>
#include <drivers/input/mouse.h>
#include <power/sleep.h>

// awful

uint8_t mouse_bytes[3];
uint8_t mouse_cycle = 0;
int32_t mouse_y = 0;
int32_t mouse_x = 0;
mouse_held_t mouse_held;

void clip_mouse(int * mouse_x, int * mouse_y) {
	if (*mouse_x < 0) {
			*mouse_x = 0;
	} else if (*mouse_x > root_window.size.width - 2) {
			*mouse_x = root_window.size.width - 2;
	}
	if (*mouse_y < 0) {
			*mouse_y = 0;
	} else if (*mouse_y > root_window.size.height - 2) {
			*mouse_y = root_window.size.height - 2;
	}
}

void mouse_handler(registers_t * regs) {
	uint8_t status = inb(PS2_STATUS);
	uint8_t mouse_in = inb(PS2_DATA);

	in_sleep_mode = 0; // wake from sleep mode if it was on

	if (status & 0x20) {
		switch (mouse_cycle++) {
			case 0:
				mouse_bytes[0] = mouse_in;
				if (!(mouse_in & 0b1000)) {
					mouse_cycle = 0;
				}
				break;
			case 1:
				mouse_bytes[1] = mouse_in;
				break;
			case 2:
				mouse_event_t * event;
				int32_t old_mouse_y = mouse_y;
				int32_t old_mouse_x = mouse_x;
				mouse_cycle = 0;
				mouse_x += mouse_bytes[1] - ((mouse_bytes[0] << 4) & 0x100);
				mouse_y -= mouse_in - ((mouse_bytes[0] << 3) & 0x100);
				clip_mouse(&mouse_x, &mouse_y);
				mouse_held.left = (mouse_bytes[0] & 0x01);
				mouse_held.right = (mouse_bytes[0] & 0x02) >> 1;
				mouse_held.middle = (mouse_bytes[0] & 0x04) >> 2;
				cursor.x = mouse_x;
				cursor.y = mouse_y;
				event = malloc(sizeof(mouse_event_t));
				event->type = EVENT_MOUSE;
				event->x = mouse_x;
				event->y = mouse_y;
				event->delta_x = mouse_bytes[1];
				event->delta_y = mouse_in;
				event->bdelta_x = mouse_x - old_mouse_x;
				event->bdelta_y = -(mouse_y - old_mouse_y);
				event->held = &mouse_held;
				broadcast_event((event_t *) event);
				break;
		}
	}
}

void mouse_write(uint8_t write) {
	keyboard_poll(KEYBOARD_INPUT);
	outb(PS2_WRITE, PS2_COMMAND_WRITE_MOUSE_INPUT);
	keyboard_poll(KEYBOARD_INPUT);
	outb(PS2_DATA, write);
}

void mouse_sample_rate(int rate) {
	mouse_write(PS2_MOUSE_SET_SAMPLERATE);
	keyboard_read();
	mouse_write(rate);
	keyboard_read();
}

uint8_t mouse_cmd(int cmd, int data, int hasdata) {
	mouse_write(cmd);
	return keyboard_read(); // discard
}

void keyboard_mouse_handle_event(event_t * event) {
	if (event->type != EVENT_KEYBOARD) {
		return;
	}
	kbd_event_t * keyevent = (kbd_event_t *) event;
	keyboard_held_t * keyheld = keyevent->held;
	mouse_event_t * mouseevent;
	int32_t old_mouse_y = 0;
	int32_t old_mouse_x = 0;
	static int left_held = 0;
	static int right_held = 0;
	static int up_held = 0;
	static int down_held = 0;

	if (((keyevent->keycode >= 0x5a && keyevent->keycode <= 0x5d) || keyheld->rshift || keyheld->rctrl || keyheld->lshift ) && keyboard_mouse) {
		int amount = (keyheld->lshift || keyheld->rshift) ? 4 : ((keyheld->lctrl) ? 16 : 8);
		switch (keyevent->keycode) {
			case 0x5a:
				up_held = keyevent->pressed;
				break;
			case 0x5b:
				right_held = keyevent->pressed;
				break;
			case 0x5c:
				left_held = keyevent->pressed;
				break;
			case 0x5d:
				down_held = keyevent->pressed;
				break;
		}
		mouse_held.left = keyheld->rctrl;
		// there HAS to be a better way to do this????
		mouseevent = malloc(sizeof(mouse_event_t));
		mouseevent->type = EVENT_MOUSE;
		mouseevent->held = &mouse_held;
		old_mouse_y = mouse_y;
		old_mouse_x = mouse_x;
		if (right_held) {
			mouseevent->delta_x += amount;
			mouse_x += amount;
		}
		if (left_held) {
			mouseevent->delta_x -= amount;
			mouse_x -= amount;
		}
		if (up_held) {
			mouseevent->delta_y -= amount;
			mouse_y -= amount;
		}
		if (down_held) {
			mouseevent->delta_y += amount;
			mouse_y += amount;
		}
		clip_mouse(&mouse_x, &mouse_y);
		cursor.x = mouse_x;
		cursor.y = mouse_y;
		mouseevent->x = mouse_x;
		mouseevent->y = mouse_y;
		mouseevent->bdelta_x = mouse_x - old_mouse_x;
		mouseevent->bdelta_y = -(mouse_y - old_mouse_y);
		broadcast_event((event_t *) mouseevent);
		return;
	}
}

uint8_t mouse_enable_z_axis() {
	uint8_t identity = 0;
	mouse_sample_rate(200);
	mouse_sample_rate(100);
	mouse_sample_rate(80);
	identity = mouse_cmd(PS2_MOUSE_IDENTIFY, 0, 0);
	return identity;
}

void mouse_disable() {
	outb(PS2_WRITE, PS2_COMMAND_DISABLE_MOUSE);
	mouse_write(PS2_MOUSE_DISABLE_IRQ);
}

void mouse_enable() {
	outb(PS2_WRITE, PS2_COMMAND_ENABLE_MOUSE);
	mouse_write(PS2_MOUSE_ENABLE_IRQ);
}

void mouse_init() {
	mouse_x = multiboot_header->framebuffer_width / 2;
	mouse_y = multiboot_header->framebuffer_height / 2;
	keyboard_poll(KEYBOARD_INPUT);
	outb(PS2_WRITE, PS2_COMMAND_ENABLE_MOUSE); // keyboard init disabled mouse, so now enable it
	keyboard_poll(KEYBOARD_INPUT);

	// enable mouse IRQ
	uint8_t status = keyboard_read_ram(0);
	keyboard_poll(KEYBOARD_INPUT);
	keyboard_write_ram(0, status | PS2_CONFIG_MOUSE_IRQ);

	// restore all the defaults and enable IRQ but again
	mouse_write(PS2_MOUSE_RESET);
	keyboard_read();
	mouse_write(PS2_MOUSE_ENABLE_IRQ);
	keyboard_read();

	// this is here now to prevent spurious interrupts during the time between interrupts are enabled and we are called
	irq_set_handler(44, mouse_handler);
	// lprintf(u"%w\n", mouse_enable_z_axis());
}

void mouse_late_init() {
	process_t * proc = create_event_zombie(u"moused", keyboard_mouse_handle_event);
	proc->system = 1;
	proc->kill = force_alive_kill_handler;
}
