bits 32
align 16

extern asm_optimial_sleep
extern yield
asm_optimial_sleep:
.loop:
	call yield
	jmp .loop