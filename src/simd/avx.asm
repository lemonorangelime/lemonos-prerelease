bits 32
align 16

extern avx_init
extern avx_supported ; from C
avx_init:
	; trash everything 😃
	mov eax, 0x01
	cpuid
	test ecx, 1 << 28
	jz .exit
	mov DWORD [avx_supported], 1
	mov eax, cr4
	or eax, 1 << 18
	mov cr4, eax
	xor ecx, ecx
	xgetbv
	or eax, 7
	xor ecx, ecx
	xsetbv
.exit:
	ret