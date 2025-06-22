#pragma once

extern uint64_t cpu_hz;
extern uint64_t cpu_khz;
extern uint64_t cpu_mhz;
extern uint64_t cpu_ghz;

void cpuspeed_wait_tsc(uint64_t offset);
void cpuspeed_measure();