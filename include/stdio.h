#pragma once

#include <stdint.h>

// circular dependency fix grahh
typedef struct process process_t;

typedef ssize_t (* stdout_handler_t)(process_t * process, uint16_t * output, size_t size);
typedef void (* stdout_constructor_t)(process_t * process);

typedef struct {
	int state;
	int buffer[8];
	int idx;
	uint32_t colour;
} stdout_state_t;

// graphics.h... through a chain of includes, depends on stdout_handler_t
#include <graphics/graphics.h>
#include <multitasking.h>

enum {
	ANSI_STATE_PRINTING = 0b00000001,
	ANSI_STATE_ESCAPE   = 0b00000010,
	ANSI_STATE_BRACKET  = 0b00000100,
	ANSI_STATE_VALUE    = 0b00001000,
	ANSI_STATE_END      = 0b00010000,
};

void ansi_stdout_constructor(process_t * process);
ssize_t ansi_stdout_print(stdout_state_t * state, uint16_t * buffer, size_t size);
ssize_t ansi_stdout_handler(process_t * process, uint16_t * buffer, size_t size);
void hook_stdout(process_t * process, stdout_handler_t handler, stdout_constructor_t constructor);

void printf(uint16_t * fmt, ...);
void cprintf(uint32_t colour, uint16_t * fmt, ...);
void lprintf(uint16_t * fmt, ...);
void iprintf(uint32_t colour, uint16_t * fmt, ...);
