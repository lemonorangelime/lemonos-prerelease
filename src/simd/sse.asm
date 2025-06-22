bits 32
align 16

extern sse_init
extern sse_supported
sse_init:
	mov eax, 0x01
	cpuid
	test edx, 1 << 25
	jz .exit
	mov DWORD [sse_supported], 1
	mov eax, cr0
	and ax, 0xfffb
	; or ax, 0x2
	mov cr0, eax
	mov eax, cr4
	or ax, 3 << 9
	mov cr4, eax
.exit:
	ret