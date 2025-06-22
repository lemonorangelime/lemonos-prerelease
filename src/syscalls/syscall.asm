bits 32
align 16

[GLOBAL fast_syscall]
[EXTERN syscall_handler]
fast_syscall:
	cli
	pusha
	push esp
	mov eax, syscall_handler
	call eax
	add esp, 4
	popa
	sti
	ret