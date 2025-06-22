#include <personalities.h>
#include <personalities/lemonos/syscall.h>
#include <interrupts/irq.h>
#include <pit.h>
#include <memory.h>
#include <sysinfo.h>
#include <string.h>
#include <stdio.h>
#include <multitasking.h>
#include <graphics/graphics.h>
#include <memory.h>
#include <bin_formats/elf.h>
#include <terminal.h>
#include <boot/initrd.h>
#include <drivers/input/mouse.h>

// LemonOS v1 code, can't do much to make this good

static personality_t personality;

void personality_lemonos_init() {
	personality_register(&personality);
}

void sysinfo_lemonos_syscall(registers_t * regs) {
	sysinfo_t * sysinfo = (sysinfo_t *) regs->ecx;
	sysinfo->uptime = ticks / pit_freq;
	sysinfo->totalram = ram_size;
	sysinfo->freeram = ram_size - used_memory;
	sysinfo->sharedram = 0;
	sysinfo->bufferram = 0;
	sysinfo->totalswap = 0;
	sysinfo->freeswap = 0;
	sysinfo->procs = proc_count;
	sysinfo->totalhigh = ram_size;
	sysinfo->freehigh = ram_size - used_memory;
	sysinfo->mem_unit = 1;
	regs->eax = 0;
}

void printf_lemonos_syscall(registers_t * regs) {
	printf((uint16_t *) regs->ecx);
	regs->eax = regs->eax;
}

void sysconf_lemonos_syscall(registers_t * regs) {
	regs->eax = -1;

	switch (regs->ecx) {
		case SYSCONF_LEMONOS_CMOSTIME:
			regs->eax = ticks / pit_freq;
			return;

		case SYSCONF_LEMONOS_TICKS:
			regs->eax = ticks;
			return;
			
		case SYSCONF_LEMONOS_FREQUENCY:
			regs->eax = pit_freq;
			return;

		case SYSCONF_LEMONOS_MEMORY_SIZE:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_MEMORY_USED:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_MEMORY_ALLOC:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_MEMORY_START:
			regs->eax = (uint32_t) heap;
			return;

		case SYSCONF_LEMONOS_MEMORY_END:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_GRAPHICS_MODE:
			regs->eax = 1;
			return;

		case SYSCONF_LEMONOS_PRIVILEGE:
			regs->eax = 1;
			return;

		case SYSCONF_LEMONOS_VERMIN:
			regs->eax = 1;
			return;

		case SYSCONF_LEMONOS_VERMAX:
			regs->eax = 32;
			return;

		case SYSCONF_LEMONOS_FPS:
			regs->eax = 60;
			return;

		case SYSCONF_LEMONOS_CLOCKHZ:
			regs->eax = pit_freq;
			return;

		case SYSCONF_LEMONOS_SCREENSAVER:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_USERNAME:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_SCREEN_WIDTH:
			regs->eax = root_window.size.width;
			return;

		case SYSCONF_LEMONOS_SCREEN_HEIGHT:
			regs->eax = root_window.size.height;
			return;

		case SYSCONF_LEMONOS_GRAPHICS_BPP:
			regs->eax = 32;
			return;

		case SYSCONF_LEMONOS_GRAPHICS_FB:
			regs->eax = (uint32_t) root_window.fb;
			return;

		case SYSCONF_LEMONOS_GRAPHICS_CURSOR:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_GRAPHICS_STREAKS:
			regs->eax = 0;
			return;

		case SYSCONF_LEMONOS_BROWSER:
			regs->eax = 0;
			return;
	}
}

void rand_lemonos_syscall(registers_t * regs) {
	regs->eax = 0;
}

void getch_lemonos_syscall(registers_t * regs) {
	*((uint16_t *) regs->ecx) = 0;
	regs->eax = regs->eax;
}

void getpid_lemonos_syscall(registers_t * regs) {
	regs->eax = (*get_current_process())->pid;
}

void gethostname_lemonos_syscall(registers_t * regs) {
	ustrcmp((uint16_t *) regs->ecx, u"LEMONOS");
	regs->eax = regs->eax;
}

void getuid_lemonos_syscall(registers_t * regs) {
	regs->eax = 0;
}

void move_cursor_lemonos_syscall(registers_t * regs) {
	regs->eax = -1;
}

void free_lemonos_syscall(registers_t * regs) {
	free((void *) regs->ecx);
	regs->eax = regs->eax;
}

void malloc_lemonos_syscall(registers_t * regs) {
	regs->eax = (uint32_t) malloc(regs->ecx);
}

void srand_lemonos_syscall(registers_t * regs) {
	regs->eax = regs->eax;
}

void system_lemonos_syscall(registers_t * regs) {
	uint16_t * string16 = (uint16_t *) regs->ecx;
	char * string = malloc(ustrlen(string16) + 1);
	ustrtoa(string, string16);

	int pid = exec_initrd(string, 0, NULL);
	if (pid == 0) {
		regs->eax = 0;
		return;
	}
	while (multitasking_get_pid(pid)) {
		yield();
	}
	regs->eax = regs->eax;
}

void putc_lemonos_syscall(registers_t * regs) {
	printf(u"%c", regs->ecx);
	regs->eax = regs->eax;
}

void open_lemonos_syscall(registers_t * regs) {
	regs->eax = -1;
}

void read_lemonos_syscall(registers_t * regs) {
	regs->eax = -1;
}

void size_lemonos_syscall(registers_t * regs) {
	regs->eax = -1;
}

void wherex_lemonos_syscall(registers_t * regs) {
	regs->eax = 0;
}

void wherey_lemonos_syscall(registers_t * regs) {
	regs->eax = 0;
}

void cprintf_lemonos_syscall(registers_t * regs) {
	cprintf(regs->edx, (uint16_t *) regs->ecx);
	regs->eax = regs->eax;
}

void cputc_lemonos_syscall(registers_t * regs) {
	cprintf(regs->edx, u"%c", regs->ecx);
	regs->eax = regs->eax;
}

void create_window_lemonos_syscall(registers_t * regs) {
	lemonos_rect_2d_t * lemonos_rect = (lemonos_rect_2d_t *) regs->ecx;
	window_t * window = gfx_create_window(u"LemonOS window", u"lemonos", lemonos_rect->width, lemonos_rect->height);
	rect_2d_t * rect = &window->rect;
	lemonos_rect->window = window;
	free(rect->fb);
	rect->fb = (uint32_t *) lemonos_rect->fb;
	lemonos_rect->fb = (unsigned char *) rect->fb;
	window->y += TERTIARY_HEIGHT;
	regs->eax = regs->eax;
}

void destroy_window_lemonos_syscall(registers_t * regs) {
	regs->eax = regs->eax;
	return; // breaks something?
	lemonos_rect_2d_t * lemonos_rect = (lemonos_rect_2d_t *) regs->ecx;
	gfx_close_window(lemonos_rect->window);
	lemonos_rect->fb = NULL;
	regs->eax = regs->eax;
}

void char_draw_lemonos_syscall(registers_t * regs) {
	lemonos_rect_2d_t * lemonos_rect = (lemonos_rect_2d_t *) regs->ecx;
	rect_2d_t * rect = &lemonos_rect->window->rect;
	uint32_t colour = regs->ebx;
	uint16_t chr = regs->edx;
	rect->cursor.x = lemonos_rect->cursorx;
	rect->cursor.y = lemonos_rect->cursory;
	gfx_char_draw(chr, lemonos_rect->cursorx, lemonos_rect->cursory, colour, rect);
	lemonos_rect->cursorx = rect->cursor.x;
	lemonos_rect->cursory = rect->cursor.y;
	regs->eax = regs->eax;
}

void mouse_buttons_lemonos_syscall(registers_t * regs) {
	regs->eax = mouse_held.left | (mouse_held.right << 1) | (mouse_held.middle << 2);
}

void mouse_x_lemonos_syscall(registers_t * regs) {
	regs->eax = cursor.x;
}

void mouse_y_lemonos_syscall(registers_t * regs) {
	regs->eax = cursor.y;
}

void poweroff_lemonos_syscall(registers_t * regs) {
	switch (regs->ecx) {
		default: regs->eax = 0; return;
		case 0: break;
		case 1: break;
	}
	regs->eax = 1;
}

void clear_screen_lemonos_syscall(registers_t * regs) {
	regs->eax = regs->eax;
}

void name_lemonos_syscall(registers_t * regs) {
	regs->eax = 0;
}

uint32_t ktask_syscall_handler(lemonos_registers_t regsstruct) {
	registers_t regs;
	regs.eax = regsstruct.eax;
	regs.ebx = regsstruct.ebx;
	regs.ecx = regsstruct.ecx;
	regs.edx = regsstruct.edx;
	syscall_handler(&regs);
}

int ktask_stub() { return 0; }
char * ktask_malloc(uint32_t size) { return malloc(size); }
uint32_t ktask_free(void * address) { return 0; }
void ktask_address_lemonos_syscall(registers_t * regs) {
	switch (regs->ecx) {
		case 0: regs->eax = (uint32_t) ktask_stub; return;
		case 1: regs->eax = (uint32_t) terminal_print; return;
		case 2: regs->eax = (uint32_t) ktask_stub; return;
		case 3: regs->eax = (uint32_t) ktask_stub; return;
		case 4: regs->eax = (uint32_t) ktask_malloc; return;
		case 5: regs->eax = (uint32_t) ktask_free; return;
		case 6: regs->eax = (uint32_t) ktask_syscall_handler; return;
		case 7: regs->eax = (uint32_t) 0; return;
		case 8: regs->eax = (uint32_t) interrupt_handlers; return;
	}
}

void ktask_panic_lemonos_syscall(registers_t * regs) {
}

static personality_t personality = {
	PERSONALITY_LEMONOSv1, u"LemonOSv1", 32,
	{
		{SYSCALL_LEMONOSv1_SYSINFO, sysinfo_lemonos_syscall},
		{SYSCALL_LEMONOSv1_PRINT, printf_lemonos_syscall},
		{SYSCALL_LEMONOSv1_SYSCONF, sysconf_lemonos_syscall},
		{SYSCALL_LEMONOSv1_RAND, rand_lemonos_syscall},
		{SYSCALL_LEMONOSv1_GETCH, getch_lemonos_syscall},
		{SYSCALL_LEMONOSv1_GETPID, getpid_lemonos_syscall},
		{SYSCALL_LEMONOSv1_GETHOSTNAME, gethostname_lemonos_syscall},
		{SYSCALL_LEMONOSv1_GETUID, getuid_lemonos_syscall},
		{SYSCALL_LEMONOSv1_MOVE_CURSOR, move_cursor_lemonos_syscall},
		{SYSCALL_LEMONOSv1_FREE, free_lemonos_syscall},
		{SYSCALL_LEMONOSv1_MALLOC, malloc_lemonos_syscall},
		{SYSCALL_LEMONOSv1_SRAND, srand_lemonos_syscall},
		{SYSCALL_LEMONOSv1_SYSTEM, system_lemonos_syscall},
		{SYSCALL_LEMONOSv1_PUTC, putc_lemonos_syscall},
		{SYSCALL_LEMONOSv1_OPEN, open_lemonos_syscall},
		{SYSCALL_LEMONOSv1_READ, read_lemonos_syscall},
		{SYSCALL_LEMONOSv1_SIZE, size_lemonos_syscall},
		{SYSCALL_LEMONOSv1_WHEREX, wherex_lemonos_syscall},
		{SYSCALL_LEMONOSv1_WHEREY, wherey_lemonos_syscall},
		{SYSCALL_LEMONOSv1_CPRINTF, cprintf_lemonos_syscall},
		{SYSCALL_LEMONOSv1_CPUTC, cputc_lemonos_syscall},
		{SYSCALL_LEMONOSv1_CREATE_WINDOW, create_window_lemonos_syscall},
		{SYSCALL_LEMONOSv1_DESTORY_WINDOW, destroy_window_lemonos_syscall},
		{SYSCALL_LEMONOSv1_CHAR_DRAW, char_draw_lemonos_syscall},
		{SYSCALL_LEMONOSv1_MOUSE_BUTTONS, mouse_buttons_lemonos_syscall},
		{SYSCALL_LEMONOSv1_MOUSE_X, mouse_x_lemonos_syscall},
		{SYSCALL_LEMONOSv1_MOUSE_Y, mouse_y_lemonos_syscall},
		{SYSCALL_LEMONOSv1_POWEROFF, poweroff_lemonos_syscall},
		{SYSCALL_LEMONOSv1_CLEAR_SCREEN, clear_screen_lemonos_syscall},
		{SYSCALL_LEMONOSv1_NAME, name_lemonos_syscall},
		{SYSCALL_LEMONOSv1_KTASK_ADDRESS, ktask_address_lemonos_syscall},
		{SYSCALL_LEMONOSv1_KTASK_PANIC, ktask_panic_lemonos_syscall}
	}
};
