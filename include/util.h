#pragma once

#include <stdint.h>

void sleep_ticks(int time);
void sleep_seconds(int time);

void reboot();
void shutdown();
void halt();
void sleep();
void disable_interrupts();
void enable_interrupts();
int interrupts_enabled();
void interrupts_restore(int flag);
void __attribute__((optimize("O0"))) halt();
void __attribute__((optimize("O0"))) hlt();

uint16_t htonw(uint16_t d);
uint32_t htond(uint32_t d);
uint64_t htonq(uint64_t d);
uint16_t ntohw(uint16_t d);
uint32_t ntohd(uint32_t d);
uint64_t ntohq(uint64_t d);