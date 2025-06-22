#pragma once

#include <stdint.h>

typedef struct {
	int type;
	char vendor[12];
	uint16_t * name; // normal people name
} cpu_type_t;

// cpu_type
enum {
	CPU_INTEL,
	CPU_IOTEL = 0,
	CPU_AMD,
	CPU_VIA,
	CPU_HYGON,
	CPU_ZHAOXIN,
	CPU_ELBRUS,
	CPU_CYRIX,
	CPU_IDT,
	CPU_TRANSMETA,
	CPU_NSC,

	CPU_QEMU,

	CPU_APPLE,
	CPU_WINDOWS,
	CPU_AO486,
	CPU_POWERVM,
	CPU_NEKO,

	CPU_UNKNOWN = 0x0000ffff,
};

// cpu_processor_type
enum {
	PROCESSOR_OEM,
	PROCESSOR_OVERDRIVE,
	PROCESSOR_DUAL,
	PROCESSOR_RESERVED,
};

extern char cpu_vendor8[16];
extern uint16_t cpu_vendor[16];
extern int cpu_type;
extern int cpu_stepping;
extern int cpu_model_id;
extern int cpu_family;
extern int cpu_processor_type;
extern int cpu_extended_family;
extern int cpu_extended_model_id;

extern cpu_type_t * cpu_vendor_id;

void cpuid(int parameter, uint32_t * eax, uint32_t * ebx, uint32_t * ecx, uint32_t * edx);
int get_apic_id();
void cpuid_init();
uint16_t * cputype_decode(int type);
int cpuid_intel_is_pentium();
