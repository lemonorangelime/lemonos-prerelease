#pragma once

#include <stdint.h>
#include <syscall.h>

typedef struct {
	int type;
	uint16_t * name;
	int syscall_count;
	syscall_t sycalls[];
} personality_t;

enum {
	PERSONALITY_LINUX		= 0x00,
	PERSONALITY_LEMONOSv1	= 0x01,
	PERSONALITY_ROADRUNNER	= 0x02,
	PERSONALITY_NT			= 0x03, // ?
	PERSONALITY_TEMPLEOS    = 0x04,
};

#define PERSONALITIES 10

extern personality_t * personality_table[PERSONALITIES];
extern int personality_top;

void personality_init();
personality_t * get_personality_by_name(uint16_t * name);
personality_t * get_personality_by_type(int type);
personality_t * get_current_personality();
void set_current_personality(personality_t * personality);
void personality_register(personality_t * personality);