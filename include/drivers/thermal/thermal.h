#pragma once

#define THERMAL_STATUS 0x19c
#define THERMAL_TARGET 0x1a2

typedef int (* thermal_read_t)();

extern thermal_read_t get_cpu_temp;

extern int therm_target;

void thermal_init();
int intel_get_cpu_temp();
int amd_get_cpu_temp();
int via_get_cpu_temp();
int amdZen_get_cpu_temp();
int amdK8_get_cpu_temp();
int fail_get_cpu_temp();