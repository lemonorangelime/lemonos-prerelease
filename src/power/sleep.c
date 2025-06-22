#include <stdint.h>
#include <stdio.h>
#include <panic.h>
#include <drivers/thermal/thermal.h>
#include <power/sleep.h>
#include <interrupts/irq.h>
#include <util.h>
#include <pit.h>
#include <graphics/graphics.h>
#include <math.h>
#include <power/perf.h>

int in_sleep_mode = 0;
int sleep_mode_schedule = 0; // schedule a sleep in the future (tick target, 0 = nothing scheduled)

// hey it works
int sleep_enter() {
	size_2d_t vga;
	size_2d_t size = back_buffer.size;
	int oldx, oldy;
	uint32_t * oldfb;
	int irq = interrupts_enabled();
	disable_interrupts(); // work in peace
	pit_disable();
	in_sleep_mode = 1;
	vga.width = size.width / 8;
	vga.height = size.height / 16;
	draw_panic_screen(0xff808080, 0, 0, 2); // draw slight grey gradient
	gfx_string_draw(u"The computer is asleep...", 1, 1, 0xffffffff, &back_buffer);
	oldx = background.cursor.x;
	oldy = background.cursor.y;
	oldfb = background.fb;
	background.cursor.x = 1;
	background.cursor.y = 2;
	background.fb = back_buffer.fb;
	rect_2d_crunch(&root_window, background.fb);
	background.fb = oldfb;
	background.cursor.x = oldx;
	background.cursor.y = oldy;
	enable_interrupts();
	while (1) {
		perf_halt(); // wait for something to happen
		if (!in_sleep_mode) {
			break; // if not in sleep mode (e.g. key pressed, mouse moved)
		}
	}
	disable_interrupts();
	sleep_mode_schedule = 0;
	pit_init(pit_freq); // restart pit
	interrupts_restore(irq);
}
