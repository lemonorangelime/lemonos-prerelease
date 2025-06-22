#pragma once

enum {
	SSE1 = 1,
	SSE2,
	SSE3,
	SSSE3,
	SSE4_1,
	SSE4_2,
	SSE4A,
};

enum {
	AVX1 = 1,
	AVX2,
	AVX512,
};

enum {
	EAX,
	EBX,
	ECX,
	EDX,
};

#define SSE4A_MASK 0b1000000
#define SSE4_2_MASK 0b100000000000000000000
#define SSE4_1_MASK 0b10000000000000000000
#define SSSE3_MASK 0b1000000000
#define SSE3_MASK 0b1
#define SSE2_MASK 0b100000000000000000000000000

#define AVX2_MASK 0b100000
#define AVX512_MASK 0

extern int avx_supported;
extern int sse_supported;

extern int fpu_level;
extern int sse_level;
extern int avx_level;

void simd_init();