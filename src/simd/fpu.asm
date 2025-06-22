bits 32
align 16

; assume FPU exists, sorry NexGen!
; 2025 - we can no longer assume FPU exists

extern fpu_init
extern fpu_level
fpu_init:
	mov eax, cr0
	and eax, 0xfffffffb
	or eax, 0b10
	mov cr0, eax
	fninit
	fnstsw [fpu_garbage]
	cmp word [fpu_garbage], 0
	jne .end

	mov DWORD [fpu_level], 1
	mov eax, cr4
	or eax, 0b11000000000
	mov cr4, eax
	fninit
.end:
	ret

fpu_garbage: dw 0x55AA
