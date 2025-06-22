#include <pit.h>
#include <stdint.h>
#include <util.h>
#include <msr.h>
#include <interrupts/apic.h>

uint64_t cpu_hz = 0;
uint64_t cpu_khz = 0;
uint64_t cpu_mhz = 0;
uint64_t cpu_ghz = 0;

void cpuspeed_wait_tsc(uint64_t offset) {
	uint64_t time = 0;
	uint64_t target = 0;
	asm volatile ("rdtsc" : "=A"(time));
	target = time + offset;
	while (time < target) {
		asm volatile ("rdtsc" : "=A"(time));
	}
}

void cpuspeed_measure() {
	uint64_t start, stop;
	uint64_t target;
	target = ticks + pit_freq;
	asm volatile ("rdtsc" : "=A"(start));
	while (ticks < target) {}
	asm volatile ("rdtsc" : "=A"(stop));
	cpu_hz = stop - start;
	cpu_khz = cpu_hz / 1000;
	cpu_mhz = cpu_hz / 1000000;
	cpu_ghz = cpu_hz / 1000000000;
}
