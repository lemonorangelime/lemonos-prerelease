#pragma once

#include <stdint.h>

typedef void (* perf_set_state_t)(int state);
typedef void (* perf_switch_t)();
typedef void (* perf_general_t)();

typedef struct {
	uint32_t fstring : 1; // 1 == fast string operations supported 0 == not
	uint32_t speedstep : 1; // 1 == speedstep supported 0 == not
	uint32_t turbo : 1; // 1 == intel turbo supported 0 == not
	uint32_t deephalt : 1; // 1 == C states over 1 supported 0 == not
	uint32_t monitorless : 1; // 1 == monitorless mwait supported 0 == not
} _perf_field_t;

typedef struct {
	union {
		_perf_field_t f;
		uint32_t i;
	};
} perf_state_t;

enum {
	// C0 ommited (you cant go to C0 cause your already in C0)
	CSTATE_C1 = 0,
	CSTATE_C2 = 1,
	CSTATE_C3 = 2,
	CSTATE_C4 = 3,
	CSTATE_C5 = 4,
	CSTATE_C6 = 5,
	CSTATE_C7 = 6,

	PSTATE_P0 = 0,
	PSTATE_P1 = 1,
	PSTATE_P2 = 2,
	PSTATE_P3 = 3,
	PSTATE_P4 = 4,
	PSTATE_P5 = 5,
};

enum {
	PERF_INTEL_MISC_FSTRING     = 0b00000000000000001,
	PERF_INTEL_MISC_PREFETCHER  = 0b00000001000000000,
	PERF_INTEL_MISC_SPEEDSTEP   = 0b10000000000000000,
	PERF_INTEL_MISC_MONITOR     = 0b1000000000000000000,
	PERF_INTEL_MISC_CACHELINE   = 0b10000000000000000000,
	PERF_INTEL_MISC_DDUPREFETCH = 0b10000000000000000000000000000000000000,
	PERF_INTEL_MISC_TURBO       = 0b100000000000000000000000000000000000000,
	PERF_INTEL_MISC_IPPREFETCH  = 0b1000000000000000000000000000000000000000,

	PERF_INTEL_CTL_TURBO        = 0b000000100000000000000000000000000000000,
};

// manually set power state (P0 - P5)
extern perf_set_state_t perf_set_pstate;

// set performance mode (speed // power save)
extern perf_switch_t perf_performance_mode;
extern perf_switch_t perf_power_save_mode;

// enable any kind of performance boosting features
// (Intel Turbo Boost, AMD Turbo core)

// enable any kind of power saving features
// (Intel SpeedStep, AMD PowerNow / "Cool'n'Quiet")

extern int perf_monitorless;
extern int perf_monitor_min;
extern int perf_monitor_max;

extern int perf_deep_halt_supported;

extern int perf_intel_fstring;
extern int perf_intel_speedstep;
extern int perf_intel_turbo;

void perf_do_nothing();
void perf_dont_switch(int state);

void perf_mwait(int state, int substate);
void perf_monitor(uint32_t eax);
void perf_set_cstate(int state);
void perf_halt();
void perf_init();