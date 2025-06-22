#include <cpuid.h>
#include <stdint.h>
#include <string.h>
#include <panic.h>
#include <util.h>

char cpu_vendor8[16];
uint16_t cpu_vendor[16];

int cpu_type = CPU_UNKNOWN;
int cpu_stepping = 0;
int cpu_model_id = 0;
int cpu_family = 0;
int cpu_processor_type = 0;
int cpu_extended_family = 0;
int cpu_extended_model_id = 0;

cpu_type_t * cpu_vendor_id;

static cpu_type_t cpu_vendor_ids[] = {
	{CPU_UNKNOWN,		"Generic x86\0",		u"Generic x86"},
	{CPU_INTEL,			"GenuineIntel",			u"Intel"},
	{CPU_IOTEL,			"GenuineIotel",			u"Intel"},
	{CPU_AMD,			"AuthenticAMD",			u"AMD"},
	{CPU_AMD,			"AMD ISBETTER",			u"AMD"},
	{CPU_VIA,			"VIA VIA VIA ",			u"VIA"},
	{CPU_HYGON,			"HygonGenuine",			u"海光 (Hygon)"},
	{CPU_ZHAOXIN,		"  Shanghai  ",			u"兆芯 (Zhaoxin)"},
	{CPU_ELBRUS,		"E2K MACHINE\0",		u"Эльбрус (Elbrus)"},
	{CPU_CYRIX,			"CyrixInstead",			u"Cyrix"},
	{CPU_IDT,			"CentaurHauls",			u"IDT"},
	{CPU_TRANSMETA,		"TransmetaCPU",			u"Transmeta"},
	{CPU_TRANSMETA,		"GenuineTMx86",			u"Transmeta"},
	{CPU_NSC,			"Geode by NSC",			u"NSC Geode"},
	{CPU_AO486,			"GenuineAO486",			u"MiSTer"},
	{CPU_AO486,			"MiSTer AO486",			u"MiSTer"},
	{CPU_POWERVM,		"PowerVM Lx86",			u"PowerVM"},
	{CPU_NEKO,			"Neko Project",			u"Neko Project"},

	{CPU_QEMU,			"Virtual QEMU",			u"Virtual QEMU"},

	// fake cpus
	{CPU_APPLE,			"VirtualApple",			u"Virtual Apple"},
	{CPU_WINDOWS,		"MicrosoftXTA",			u"Virtual Microsoft"}
};

int cpu_vendor_ids_len = sizeof(cpu_vendor_ids) / sizeof(cpu_vendor_ids[0]);

void cpuid(int parameter, uint32_t * eax, uint32_t * ebx, uint32_t * ecx, uint32_t * edx) {
	asm volatile("cpuid"

		: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "a"(parameter)
	);
}

int cpuid_intel_is_pentium() {
	if (cpu_family != 15) {
		return 0;
	}
	if (cpu_model_id < 5) {
		return 1;
	}
	if (cpu_model_id == 6) {
		return 1;
	}
	return 0;
}

int cpuid_find_type() {
	// figure out what this shit is
	cpu_vendor_id = &cpu_vendor_id[0];
	for (int i = 0; i < cpu_vendor_ids_len; i++) {
		cpu_type_t * id = &cpu_vendor_ids[i];
		if (memcmp(cpu_vendor8, id->vendor, 12) == 0) {
			cpu_vendor_id = id;
			return cpu_type = id->type;
		}
	}
}

int cpuid_find_processor_info() {
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, &eax, &ebx, &ecx, &edx);
	cpu_stepping           = (eax & 0b00000000000000000000000000001111);
	cpu_model_id           = (eax & 0b00000000000000000000000011110000) >> 4;
	cpu_family             = (eax & 0b00000000000000000000111100000000) >> 8;
	cpu_processor_type     = (eax & 0b00000000000000000011000000000000) >> 12;
	cpu_extended_model_id  = (eax & 0b00000000000011110000000000000000) >> 16;
	cpu_extended_family    = (eax & 0b00001111111100000000000000000000) >> 20;
	if (cpu_family == 15 || cpu_family == 6) {
		cpu_model_id += cpu_extended_model_id << 4;
	}
	if (cpu_family == 15) {
		cpu_family += cpu_extended_family;
	}
}

uint16_t * cputype_decode(int type) {
	switch (type) {
		case CPU_INTEL:
			return u"CPU_INTEL";
		case CPU_AMD:
			return u"CPU_AMD";
		case CPU_VIA:
			return u"CPU_VIA";
		case CPU_HYGON:
			return u"CPU_HYGON";
		case CPU_ZHAOXIN:
			return u"CPU_ZHAOXIN";
		case CPU_ELBRUS:
			return u"CPU_ELBRUS";
		case CPU_CYRIX:
			return u"CPU_CYRIX";
		case CPU_IDT:
			return u"CPU_IDT";
		case CPU_TRANSMETA:
			return u"CPU_TRANSMETA";
		case CPU_NSC:
			return u"CPU_NSC";
		case CPU_QEMU:
			return u"CPU_QEMU";
		case CPU_APPLE:
			return u"CPU_APPLE";
		case CPU_WINDOWS:
			return u"CPU_WINDOWS";
		case CPU_UNKNOWN:
			return u"CPU_UNKNOWN";
	}
}

void cpuid_init() {
	uint32_t eax;
	cpuid(0, &eax, (uint32_t *) (cpu_vendor8), (uint32_t *) (cpu_vendor8 + 8), (uint32_t *) (cpu_vendor8 + 4));
	cpu_vendor8[12] = 0;
	atoustr(cpu_vendor, cpu_vendor8);
	cpuid_find_processor_info();
	cpuid_find_type();
}
