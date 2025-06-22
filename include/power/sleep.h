#pragma once

extern int in_sleep_mode;
extern int sleep_mode_schedule;

int sleep_enter();
int sleep_wake();
void asm_optimial_sleep();