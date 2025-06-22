#pragma once

extern int32_t mouse_y;
extern int32_t mouse_x;
extern mouse_held_t mouse_held;

void mouse_init();
void mouse_late_init();
void mouse_disable();
void mouse_enable();
void clip_mouse(int * mouse_x, int * mouse_y);

// mouse identities
enum {
	MOUSE, // nothing special
	MOUSE_WITH_SCROLL = 0x03, // scroll wheel having mouse
	MULTI_BUTTON_MOUSE, // scoll wheel and 5 button
};