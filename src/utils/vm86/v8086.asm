bits 32
align 16
extern v8086_supported
extern v8086_init
v8086_init:
	ret


extern v8086_enter
v8086_enter:
	mov eax, esp
	push ds
	push eax
	pushf
	pop edx
	or edx, 1 << 17
	push edx
	popf
	push cs
	mov eax, [eax + 0x04]
	push eax
	iret

align 4
bits 16
extern v8086_test
v8086_test:
.loop
	hlt
	jmp .loop
