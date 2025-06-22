bits 32
align 16
[EXTERN mce_init]
extern mce_supported
mce_init:
	mov eax, 1
	cpuid
	test edx, 1 << 7
	jz .exit
	mov [mce_supported], eax
	mov eax, cr4
	or ax, 1 << 6
	mov cr4, eax
.exit:
	ret