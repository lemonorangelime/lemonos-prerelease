#include <stdint.h>
#include <msr.h>
#include <drivers/thermal/thermal.h>
#include <cpuid.h>
#include <bus/pci.h>

thermal_read_t get_cpu_temp = fail_get_cpu_temp;

int therm_target = 100;

int thermal_intel_cpu_quirks() {
	if (cpu_type == CPU_INTEL && cpu_model_id == 15 && cpu_family == 6) {
		return 1; // intel core 2
	}
	return 0; // no quirks
}

void thermal_init() {
	switch (cpu_type) {
		case CPU_QEMU:
			get_cpu_temp = fail_get_cpu_temp;
			return;
		case CPU_ELBRUS:
		case CPU_INTEL:
			if (cpuid_intel_is_pentium()) {
				return;
			}
			get_cpu_temp = intel_get_cpu_temp;
			if (thermal_intel_cpu_quirks()) {
				return;
			}
			uint64_t value = cpu_read_msr(0x1a2);
			therm_target = (value >> 16) & 0x7f;
			return;
		case CPU_HYGON:
		case CPU_AMD:
			if (cpu_extended_family >= 8) {
				get_cpu_temp = amdZen_get_cpu_temp;
				return; // no way for me to test this :shrug:
			}
			get_cpu_temp = (cpu_extended_family == 0) ? amdK8_get_cpu_temp : amd_get_cpu_temp;
			return;
		case CPU_ZHAOXIN:
		case CPU_CYRIX:
		case CPU_VIA:
			get_cpu_temp = via_get_cpu_temp;
			return;
	}
}

int intel_get_cpu_temp() {
	uint64_t value;
	value = cpu_read_msr(0x19c);
	// tempature returned is a differential from thermal target
	return therm_target - ((value >> 16) & 0x7f);
}

int amdZen_get_cpu_temp() {
	return 0; // I dont own a zen cpu sooooo
}

int amd_get_cpu_temp() {
	uint32_t therm_reg = pci_config_ind(0, 24, 3, 0xa4);
	int temp = ((therm_reg >> 21) & 0x7ff) / 8;
	return temp;
}

int amdK8_get_cpu_temp() {
	uint32_t therm_reg = pci_config_ind(0, 24, 3, 0xe4);
	int temp = ((therm_reg >> 16) & 0xff) - 49;
	return temp;
}

int via_get_cpu_temp() {
	if (cpu_family == 0x07 && cpu_model_id == 0x0f) {
		return cpu_read_msr(0x1423) & 0xffffff;
	}
	if (cpu_model_id == 0x0a && cpu_model_id == 0x0d) {
		return cpu_read_msr(0x1169) & 0xffffff;
	}
	return 0;
}

int fail_get_cpu_temp() {
	return 0;
}
