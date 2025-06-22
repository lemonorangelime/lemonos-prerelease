bits 32
align 16

[EXTERN tce_init]
tce_init:
	mov eax, 0x80000001
	cpuid
	test ecx, 1 << 17
	jz .exit
	mov ecx, 0xc0000080
	rdmsr
	or eax, 1 << 15
	wrmsr
.exit:
	ret