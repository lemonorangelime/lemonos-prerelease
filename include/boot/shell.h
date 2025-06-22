#pragma once

#include <stdint.h>

typedef int (* main_t)(int argc, uint16_t ** argv);

typedef struct {
	uint16_t * text;
	main_t main;
} cmd_t;

extern cmd_t cmds[];
extern size_t cmd_len;

int ksystem(uint16_t * cmd);