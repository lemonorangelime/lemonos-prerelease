#include <util.h>
#include <interrupts/irq.h>
#include <graphics/graphics.h>
#include <pit.h>
#include <asm.h>
#include <panic.h>
#include <string.h>
#include <stdint.h>
#include <version.h>
#include <input/input.h>
#include <multitasking.h>
#include <stdio.h>
#include <boot/boot.h>
#include <msr.h>
#include <errors/mce.h>

int ignore_mce = 0;

uint32_t draw_panic_screen(uint32_t colour, uint32_t target, uint32_t mask, float divisor) {
	int repeats = (int) ((1 + (back_buffer.size.height / 255)) * divisor);
	for (int i = 0; i < back_buffer.size.height; i++) {
		uint32_t * line = back_buffer.fb + (i * back_buffer.size.width);
		uint32_t fillcolour = 0;
		uint32_t oldcolour = colour;
		if (i < (255 * repeats)) {
			if (i % repeats == 0) {
				colour = rgb_degrade(colour, target);
			}
			fillcolour = colour;
		}

		int found = 0;
		for (int i = 0; i < back_buffer.size.width; i++) {
			if (mask && mask != *line) {
				line++;
				continue;
			}
			*line++ = fillcolour;
			found = 1;
		}
		if (!found) {
			colour = oldcolour;
		}
	}
	return colour;
}

static uint32_t * print_fxsave(uint32_t * fxsave, int x, int line) {
	int i = 5;
	background.cursor.x = x;
	background.cursor.y = line;
	while (i) {
		cprintf(0xffffffff, u" %r", *fxsave++);
		i--;
	}
	return fxsave;
}

// im so fuckibng sorry this is awful
void panic(int error, void * p) {
	if (sensitive_mode) {
		sensitive_panic(error, p);
	}
	disable_interrupts();
	pit_disable();
	asm volatile("cli");
	asm volatile("finit");
	gfx_init_done = 0;
	proc_count = 1;
	size_2d_t size = back_buffer.size;
	size_2d_t vga;
	registers_t * regs = p;
	process_t * current_process = *get_current_process();
	process_t * active_process = *get_active_process();
	uint32_t esp = (p == NULL) ? current_process->esp : regs->esp;
	int line = 2;
	draw_panic_screen(0xffff0000, 0, 0, 1);
	vga.width = size.width / 8;
	vga.height = size.height / 16;
	background.fb = back_buffer.fb;
	background.size = size;
	gfx_string_draw(u"KERNEL PANIC", (vga.width / 2) - 6, 2, 0xffffffff, &back_buffer);
	line += 3;
	gfx_string_draw(u"ERROR: ", 4, line, 0xffffffff, &back_buffer);
	gfx_string_draw(error_name(error), 11, line, 0xffffffff, &back_buffer);
	line += 3;
	if (p == 0) {
		gfx_string_draw(u"REGISTERS UNKNOWN", 4, line, 0xffffffff, &back_buffer);
	} else {
		background.cursor.x = 4;
		background.cursor.y = line;
		cprintf(0xffffffff, u"EAX=%r EBX=%r ECX=%r EDX=%r", regs->eax, regs->ebx, regs->ecx, regs->edx);
		background.cursor.x = 4;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"ESI=%r EDI=%r EBP=%r ESP=%r", regs->esi, regs->edi, regs->ebp, regs->esp);
		background.cursor.x = 4;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"FLG=%r EIP=%r ERR=%r INT=%r", regs->eflags, regs->eip, regs->err_code, regs->int_no);
	}
	line += 2;
	if (multitasking_done) {
		uint32_t * proc_fxsave = current_process->fpusave;
		gfx_string_draw(u"Current Process:", 4, line, 0xffffffff, &back_buffer);
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"%d        ", current_process->pid);
		cprintf(0xffffffff, u"%s        ", current_process->name);
		cprintf(0xffffffff, u"%s (Current)", current_process->system ? u"(System Process)" : u""); /*
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"%d        ", active_process->pid);
		cprintf(0xffffffff, u"%s        ", active_process->name);
		cprintf(0xffffffff, u"%s (Active)", active_process->system ? u"(System Process)" : u"");*/
		line++;
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"FPUSAVE BUFFER: (%r)", current_process->fpusave);
		proc_fxsave = print_fxsave(proc_fxsave, 7, ++line);
		proc_fxsave = print_fxsave(proc_fxsave, 7, ++line);
		proc_fxsave = print_fxsave(proc_fxsave, 7, ++line);
		line++;
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"Allocated stack: %r-%r (%u)", current_process->stack, ((uint32_t) current_process->stack) - current_process->stack_size, current_process->stack_size);
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"Stack usage: %d (guess)", ((uint32_t) current_process->stack) - esp);
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"Windows: %r, Allocations: %r", current_process->windows, current_process->allocs);
		background.cursor.x = 6;
		background.cursor.y = ++line;
		cprintf(0xffffffff, u"Allocated quantum: %u", current_process->quantum);
	} else {
		background.cursor.x = 4;
		background.cursor.y = line;
		cprintf(0xffffffff, u"NO MULTITASKER PRESENT");
	}

	if (error == UNSUPPORTED_PROCESSOR) {
		background.cursor.x = 4;
		background.cursor.y = line + 2;
		cprintf(0xffffffff, u"Please use a supported CPU, such as a VIA, AMD, Intel, ");
	}

	gfx_string_draw(u"Please contact support@lemob.xyz with this error so it may be resolved", (vga.width / 2) - 35, vga.height - 3, 0xffffffff, &back_buffer);
	background.cursor.x = 0;
	background.cursor.y = vga.height - 1;
	cprintf(0xffffffff, u"LemonOS v%d.%d.%d.%d (%s)", ver_edition, ver_major, ver_minor, ver_patch, os_name16);
	rect_2d_crunch(&root_window, back_buffer.fb);
	halt();
}

void mce_panic(int error, void * p) {
	if (sensitive_mode) {
		sensitive_panic(error, p);
	}
	if (ignore_mce) {
		return;
	}
	disable_interrupts();
	pit_disable();
	asm volatile("cli");
	asm volatile("finit");
	gfx_init_done = 0;
	proc_count = 1;
	size_2d_t size = back_buffer.size;
	size_2d_t vga;
	uint64_t addr = 0, type = 0;
	uint32_t addr_high = 0, addr_low = 0;
	uint32_t type_high = 0, type_low = 0;
	registers_t * regs = p;
	int line = 2;
	uint32_t text_color = 0xffffffff;
	draw_panic_screen(0xff008000, 0, 0, 2); // 0xff40ee20
	vga.width = size.width / 8;
	vga.height = size.height / 16;
	background.fb = back_buffer.fb;
	background.size = size;
	addr = cpu_read_msr(0);
	type = cpu_read_msr(1);
	addr_low = addr & 0xffffffff;
	addr_high = (addr >> 32) & 0xffffffff;
	type_low = type & 0xffffffff;
	type_high = (type >> 32) & 0xffffffff;
	gfx_string_draw(u"KERNEL PANIC // HARDWARE FAILURE", (vga.width / 2) - 16, 2, text_color, &back_buffer);
	line += 2;
	background.cursor.x = 4;
	background.cursor.y = ++line;
	cprintf(0xffffffff, u"ADDR: %rh %rl, TYPE: %rh %rl", addr_high, addr_low, type_high, type_low);
	gfx_string_draw(u"A CRITICAL HARDWARE EXCEPTION HAS OCCURRED", 4, line += 2, text_color, &back_buffer);
	gfx_string_draw(u"CONTACT YOUR HARDWARE VENDOR", 4, ++line, text_color, &back_buffer);
	line++;
	gfx_string_draw(u"SE HA OCURRIDO UNA EXCEPCIÓN DE HARDWARE CRÍTICO", 4, ++line, text_color, &back_buffer);
	gfx_string_draw(u"CONTACTE A SU VENDEDOR DE HARDWARE", 4, ++line, text_color, &back_buffer);
	line++;
	gfx_string_draw(u"ES IST EINE KRITISCHE HARDWARE-AUSNAHME AUFGETRETEN", 4, ++line, text_color, &back_buffer);
	gfx_string_draw(u"KONTAKTIEREN SIE IHREN HARDWARE-HÄNDLER", 4, ++line, text_color, &back_buffer);
	line++;
	gfx_string_draw(u"ПРОИЗОШЛА ЖЁСТКАЯ ПОЛОМКА ЖЕЛЕЗА", 4, ++line, text_color, &back_buffer);
	gfx_string_draw(u"ОБРАТИТЕСЬ К БАРЫГАМ ЗА ЗАПЧАСТЯМИ.", 4, ++line, text_color, &back_buffer);
	line++;
	gfx_string_draw(u"ΕΧΕΙ ΣΥΜΒΕΙ ΚΡΙΣΙΜΗ ΕΞΑΙΡΕΣΗ ΥΛΙΚΟΥ", 4, ++line, text_color, &back_buffer);
	gfx_string_draw(u"ΕΠΙΚΟΙΝΩΝΗΣΤΕ ΜΕ ΤΟΝ ΠΡΟΜΗΘΕΥΤΗ ΥΛΙΚΟΥ ΣΑΣ", 4, ++line, text_color, &back_buffer);
	line++;
	gfx_string_draw(u"중요한 하드웨어 예외가 발생했습니다", 4, ++line, text_color, &back_buffer);
	gfx_string_draw(u"하드웨어 공급업체에 문의", 4, ++line, text_color, &back_buffer);

	gfx_string_draw(u"IF YOU BELIEVE THIS TO BE IN ERROR, CHOOSE nomce IN ADVANCED OPTIONS", (vga.width / 2) - 34, vga.height - 4, 0xffffffff, &back_buffer);

	gfx_string_draw(u"Contact support@lemob.xyz for more information", (vga.width / 2) - 23, vga.height - 3, 0xffffffff, &back_buffer);
	background.cursor.x = 0;
	background.cursor.y = vga.height - 1;
	cprintf(0xffffffff, u"LemonOS v%d.%d.%d.%d (%s)", ver_edition, ver_major, ver_minor, ver_patch, os_name16);
	rect_2d_crunch(&root_window, back_buffer.fb);
	halt();
}

void sensitive_panic(int error, void * p) {
	disable_interrupts();
	pit_disable();
	asm volatile("cli");
	asm volatile("finit");
	gfx_init_done = 0;
	proc_count = 1;
	size_2d_t size = back_buffer.size;
	size_2d_t vga;
	int line = 2;
	uint32_t text_color = 0xffffffff;
	uint32_t bottom = draw_panic_screen(0xffff00ff, 0, background_colour, 1); // 0xff40ee20
	memset32(((void *) back_buffer.fb) + taskbar_y_offset, bottom, taskbar_size);
	vga.width = size.width / 8;
	vga.height = size.height / 16;
	background.fb = back_buffer.fb;
	background.size = size;
	gfx_string_draw(u"SENSITIVE MODE", (vga.width / 2) - 7, vga.height - 1, text_color, &back_buffer);
	gfx_string_draw(error_name(error), 0, vga.height - 1, text_color, &back_buffer);
	rect_2d_crunch(&root_window, back_buffer.fb);
	halt();
}

uint32_t getcr(int cr) {
	uint64_t r = 0;
	return 0;
}

uint16_t * error_name(int error) {
	switch (error) {
		default:
			return u"UNKNOWN_ERROR";
		case MANUAL_PANIC:
			return u"MANUAL_PANIC";
		case OUT_OF_MEMORY:
			return u"OUT_OF_MEMORY";
		case MEMORY_CORRUPTION:
			return u"MEMORY_CORRUPTION";
		case GENERAL_MEMORY:
			return u"GENERAL_MEMORY";
		case GENERAL_PROTECTION:
			return u"GENERAL_PROTECTION";
		case GENERAL_PAGE:
			return u"GENERAL_PAGE";
		case GENERAL_ARITHMATIC:
			return u"GENERAL_ARITHMATIC";
		case UNKNOWN_OPCODE:
			return u"UNKNOWN_OPCODE";
		case DEVICE_UNAVAILABLE:
			return u"DEVICE_UNAVAILABLE";
		case UNKNOWN_FATAL_ERROR:
			return u"UNKNOWN_FATAL_ERROR";
		case SHUTDOWN_FAILURE:
			return u"SHUTDOWN_FAILURE";
		case MULTIBOOT_ERROR:
			return u"MULTIBOOT_ERROR";
		case ACPI_FATAL_ERROR:
			return u"ACPI_FATAL_ERROR";
		case GRAPHICS_ERROR:
			return u"GRAPHICS_ERROR";
		case MULTIPROCESSING_FAILURE:
			return u"MULTIPROCESSING_FAILURE";
		case COPROCESSOR_FATAL:
			return u"COPROCESSOR_FATAL";
		case MCE_HARDWARE_FAILURE:
			return u"MCE_HARDWARE_FAILURE";
		case DEBUG_EXCEPTION:
			return u"DEBUG_EXCEPTION";
		case THERM_CATASTROPHIC_SHUTDOWN:
			return u"THERM_CATASTROPHIC_SHUTDOWN";
		case RECIEVED_64BIT_POINTER:
			return u"RECIEVED_64BIT_POINTER";
		case UNSUPPORTED_PROCESSOR:
			return u"UNSUPPORTED_PROCESSOR";
		case SYSTEM_FILE_MISSING:
			return u"SYSTEM_FILE_MISSING";
	}
}

void handle_error(int error, void * p) {
	disable_interrupts();
	switch (error) {
		case MULTIBOOT_ERROR:
			// cant do anything about this
			halt();
			break;
	
		case GRAPHICS_ERROR:
			// cant do anything about this
			halt();
			break;
	
		case RECIEVED_64BIT_POINTER:
			// cant do anything about this
			halt();
			break;

		default:
			panic(error, p);
			break;
	}
}

void __attribute__ ((noreturn)) __stack_chk_fail_local(void) {
	panic(MCE_HARDWARE_FAILURE, NULL);
	while (1 == 1) {
	        asm volatile ("nop");
	}
}
