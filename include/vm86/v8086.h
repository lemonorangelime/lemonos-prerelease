#pragma once

extern int v8086_supported;

void v8086_emulate(registers_t * regs);
void v8086_init2();

// v8086_2.asm
void v8086(void * eip);
void v8086_init();