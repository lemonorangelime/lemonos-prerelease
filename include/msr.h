#pragma once

#include <stdint.h>

enum {
	MSR_MCE_ADDRESS = 0x0, // holds the address of a machine check (e.g: memory at 0x7b5c has exploded)
	MSR_MCE_TYPE = 0x1, // holds type of machine check
	MSR_INTEL_SERIAL = 0x03, // the cpu's serial
	MSR_TIMESTAMP_COUNTER = 0x10, // holds clock cycles since boot (rdtsc)
	MSR_SPECULATION_CTRL = 0x48, // stuff to control how the processor speculates branches
	MSR_SPECULATION_CMD = 0x49,
	MSR_INTEL_PROTECTED_SERIAL = 0x4f, // the serial again
	MSR_APIC_BASE = 0x1b, // address of APIC
	MSR_FEATURE_CTRL = 0x3a, // "Control Features in Intel 64 Processor" according to manual
	MSR_PLATFORM_INFO = 0xce, // no clue what this actually does ?
	MSR_INTEL_MODEL30_CSTATE_CTRL = 0xe2, // controls how cstates work
	MSR_VULNERABILITIES = 0x10a, // vulnerability enumeration
	MSR_INTEL_TSX_CTRL = 0x122, // this controls transactional memory
	MSR_INTEL_MCU_CTRL = 0x123, // control vulnerability mitigations
	MSR_MISC = 0x140, // more miscilaniious stuff
	MSR_SYSTENTER_ESP = 0x175, // ESP to use and EIP to jump to when executing sysenter
	MSR_SYSTENTER_EIP = 0x176,
	MSR_INTEL_TEMPATURE = 0x19c, // the cpu's tempature
	MSR_INTEL_PERF_CTRL = 0x199, // performance features
	MSR_INTEL_MISC = 0x1a0, // some random bits to control a bunch of features
	MSR_INTEL_TJMAX_TARGET = 0x1a2, // the cpu's target tempature
	MSR_INTEL_MISC_MODERN = 0x1a4, // new address of MISC on modern processors
	MSR_PM_ENABLE = 0x770, // pm == power management
	MSR_PM_CAP = 0x771, // capabilities
	MSR_PM_IRQ = 0x773, // irq enable / disable
	MSR_PM_HINTS = 0x774, // hints to the power managment hardware
	MSR_PM_STATUS = 0x777, // ththe status
	MSR_VIA_AIS_FLAGS = 0x1107, // VIA Alternate Instruction Set flags register
	MSR_VIA_FAM10_TEMPATURE = 0x1169, // VIA tempature sensors registers on family 10 and 7
	MSR_VIA_FAM7_TEMPATURE = 0x1423,
	MSR_LONG_MODE = 0xc0000080, // feature control for long mode
	MSR_SYSCALL_FLAGS = 0xc0000081, // like sysenter, EIP to jump to on syscall
	MSR_AMD_PROCESSOR_NAME = 0xc0010030, // amd throws the processor's name over here
	MSR_AMD_PROTECTED_SERIAL = 0xc00102f1f, // protected serial, amd version
};

uint64_t cpu_read_msr(uint32_t ecx);
void cpu_write_msr(uint32_t ecx, uint64_t value);
int asm_read_msr(uint32_t ecx, uint32_t * eax, uint32_t * edx);