#include <power/perf.h>
#include <cpuid.h>
#include <power/sleep.h>
#include <stdint.h>
#include <util.h>
#include <msr.h>

perf_set_state_t perf_set_pstate = perf_dont_switch;

perf_switch_t perf_performance_mode = perf_do_nothing;
perf_switch_t perf_power_save_mode = perf_do_nothing;

void perf_do_nothing() {}
void perf_dont_switch(int state) {}

int perf_monitorless = 0;
int perf_monitor_min = 0;
int perf_monitor_max = 0;

int perf_deep_halt_supported = 0;

// intel
int perf_intel_fstring = 0;
int perf_intel_speedstep = 0;
int perf_intel_turbo = 0;

void perf_mwait(int state, int substate) {
	uint32_t eax = 0;
	if (!perf_deep_halt_supported) {
		hlt(); // emulate with hlt
		return;
	}
	eax = (state & 0b111) << 4;
	eax |= substate & 0b111;
	asm volatile("mwait"
		:: "a"(eax), "c"(0)
	);
}

void perf_monitor(uint32_t eax) {
	if (!perf_deep_halt_supported) {
		return; // :shrug:
	}
	asm volatile("monitor"
		:: "a"(eax), "c"(0), "d"(0)
	);
}

void perf_set_cstate(int state) {
	if (perf_deep_halt_supported) {
		perf_monitor((uint32_t) &in_sleep_mode);
		perf_mwait(state, 0);
		return;
	}
	hlt(); // emulate with hlt
}

void perf_halt() {
	if (perf_deep_halt_supported) {
		perf_set_cstate(CSTATE_C6);
		return;
	}
	hlt(); // same as above, emulate with hlt
}

void perf_intel_enable_feats() {
	uint32_t eax, ebx, ecx, ecx1;
	int trash; // point unwanted registers here
	uint64_t reg;
	cpuid(6, &eax, &trash, &trash, &trash);
	cpuid(7, &trash, &ebx, &trash, &trash);
	cpuid(1, &trash, &trash, &ecx, &trash);
	reg = cpu_read_msr(0x1a0);
	if (ebx & 0b01000000000 != 0) { // check for enhanced string shjit
		perf_intel_fstring = 1;
		reg |= PERF_INTEL_MISC_FSTRING; // enable enhanced string intructions
	}
	if (ecx & 0b010000000 != 0) { // check for speedstep
		perf_intel_speedstep = 1;
		reg |= PERF_INTEL_MISC_SPEEDSTEP; // enable speedstep
	}
	if (eax & 0b10 != 0) { // check if turbo is supported
		reg ^= reg & PERF_INTEL_MISC_TURBO; // enable turbo
	}
	if (perf_deep_halt_supported) { // check if monitor is supported
		reg |= PERF_INTEL_MISC_MONITOR; // enable turbo
	}
	reg ^= reg & PERF_INTEL_MISC_PREFETCHER; // enable prefetchers (nothing happens if they are unsupported)
	reg ^= reg & PERF_INTEL_MISC_CACHELINE;
	reg ^= reg & PERF_INTEL_MISC_DDUPREFETCH;
	reg ^= reg & PERF_INTEL_MISC_IPPREFETCH;
	reg |= PERF_INTEL_MISC_MONITOR;
	cpu_write_msr(0x1a0, reg);

	if (eax & 0b10 == 0) {
		return; // dont continue enabling turbo if its not supported
	}
	perf_intel_turbo = 1;

	reg = cpu_read_msr(0x199);
	reg ^= reg & PERF_INTEL_CTL_TURBO; // enable "Intel® Dynamic Acceleration Technology"
	cpu_write_msr(0x199, reg);
}

int perf_enable_feats() {}

void perf_init() {
	uint32_t eax, ebx, ecx, edx;
	if (cpu_type != CPU_INTEL && cpu_type != CPU_AMD) {
		return; // dont support you :c
	}
	cpuid(1, &eax, &ebx, &ecx, &edx);
	perf_deep_halt_supported = (ecx >> 3) & 1;
	if (perf_deep_halt_supported) {
		cpuid(5, &eax, &ebx, &ecx, &edx);
		perf_monitorless = (ecx >> 3) & 1;
		perf_monitor_max = ebx & 1;
		perf_monitor_min = eax & 1;
	}
	perf_enable_feats(); // enable non processor specific features
	if (cpu_type == CPU_INTEL) {
		perf_intel_enable_feats(); // enable intel specific features
		return;
	}
}
