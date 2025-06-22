#include <cpuid.h>
#include <simd.h>
#include <stdint.h>

int sse_supported = 0;
int avx_supported = 0;

int fpu_level = -1;
int sse_level = -1;
int avx_level = -1;

int simd_check_cpuid(int page, uint32_t mask, uint32_t bits, int level, int reg, int * out) {
	uint32_t trash, flag, eax, ebx, ecx, edx;
	cpuid(page, &trash, &trash, &ecx, &edx);
	switch (reg) {
		case EAX:
			flag = eax;
			break;
		case EBX:
			flag = ebx;
			break;
		case ECX:
			flag = ecx;
			break;
		case EDX:
			flag = edx;
			break;
	}
	if ((flag & mask) == bits && *out == -1) {
		*out = level;
	}
}

void simd_test_sse() {
	simd_check_cpuid(0x01, SSE4A_MASK, SSE4A_MASK, SSE4A, ECX, &sse_level);
	simd_check_cpuid(0x01, SSE4_2_MASK, SSE4_2_MASK, SSE4_2, ECX, &sse_level);
	simd_check_cpuid(0x01, SSSE3_MASK, SSSE3_MASK, SSSE3, ECX, &sse_level);
	simd_check_cpuid(0x01, SSE3_MASK, SSE3_MASK, SSE3, ECX, &sse_level);
	simd_check_cpuid(0x01, SSE2_MASK, SSE2_MASK, SSE2, EDX, &sse_level);
	if (sse_level == -1) {
		sse_level = SSE1; // if its none of those above it has to be SSE1
	}
}

void simd_test_avx() {
	simd_check_cpuid(0x07, AVX2_MASK, AVX2_MASK, AVX2, EBX, &avx_level);
	if (avx_level == -1) {
		avx_level = AVX1; // if its not AVX2 it has to be AVX1 (AVX512 impossible)
	}
}

void simd_init() {
	if (sse_supported) {
		simd_test_sse();
	}
	if (avx_supported) {
		simd_test_avx();
	}
}