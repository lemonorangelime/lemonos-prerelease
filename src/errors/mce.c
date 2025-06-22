#include <errors/mce.h>
#include <cpuid.h>
#include <stdint.h>

int mce_supported = 0;

int mce_address_reg = 0;
int mce_status_reg = 1;

void mce2_init() {
	uint32_t eax, ebx, ecx, edx;
	if (!mce_supported) {
		return; // nothing to do
	}
	cpuid(5, &eax, &ebx, &ecx, &edx);
	if ((edx << 14) & 1) {
		mce_address_reg = 0x402;
		mce_status_reg = 0x401;
	}
}
