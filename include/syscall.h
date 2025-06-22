#pragma once

#include <asm.h>

typedef void (* syscall_enter_t)(registers_t *);

typedef struct {
	int id;
	syscall_enter_t enter;
} syscall_t;

enum {
	SYSCALL_PERSONALITY = 285,
};
void init_syscall();
void syscall_handler(registers_t * regs);

// not in syscall.c
void fast_syscall();
void syscall_entry();
