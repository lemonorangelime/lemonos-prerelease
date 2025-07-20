#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <interrupts/irq.h>
#include <string.h>
#include <multitasking.h>
#include <syscall.h>
#include <personalities.h>

void personality_syscall(registers_t * regs);

// find a syscall struct by its id
static inline syscall_enter_t syscall_search(uint32_t id) {
	personality_t * personality = get_current_personality();
	size_t i = personality->syscall_count;
	syscall_t * table = personality->sycalls;
	while (i--) {
		if (table[i].id == id) {
			return table[i].enter;
		}
	}
	if (id == SYSCALL_PERSONALITY) {
		return personality_syscall;
	}
	return NULL;
}

void syscall_handler(registers_t * regs) {
	syscall_enter_t enter = syscall_search(regs->eax); // find syscall struct
	if (!enter) {
		regs->eax = -1; // if failed then return -1
		return;
	}
	enter(regs); // enter syscall
}

void init_syscall() {
	// move syscall handler to fixed address, this lets us do `call 0x600` instead of
	// an expensive `int 0x80`
	memcpy32((void *) 0x600, (void *) fast_syscall, 32);
	irq_set_handler(128, syscall_handler);
}

void personality_syscall(registers_t * regs) {
	switch (regs->ebx) {
		case 0:
			personality_t * personality = get_personality_by_type(regs->ecx);
			set_current_personality(personality);
			return;
		case 1:
			regs->eax = get_current_personality()->type;
			return;
		case 2:
			process_t * process = multitasking_get_pid(regs->ecx);
			
			return;
	}
}
