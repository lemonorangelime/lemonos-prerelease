#pragma once

#include <stdint.h>

typedef struct {
	uint32_t edi, esi, ebp, trash, ebx, edx, ecx, eax;
	uint32_t int_no, err_code;
	uint32_t eip, cs, eflags, esp, ss;
} registers_t;