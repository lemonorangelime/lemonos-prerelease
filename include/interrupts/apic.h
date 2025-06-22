#pragma once

#include <stdint.h>

extern uint32_t apic_address;
extern int * cpus_booted;
extern int apic_supported;
extern int madt_broken;
extern uint32_t * multicpu_stack;
extern volatile int protected_lock;
extern int cpus;
extern int multicore_enabled;

void apic_init();
