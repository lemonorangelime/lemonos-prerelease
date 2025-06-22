bits 32
align 16

[EXTERN cache_init]
cache_init:
	mov eax, cr0
	and eax, 0x0fffffff
	mov cr0, eax
	ret