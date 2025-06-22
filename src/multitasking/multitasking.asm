bits 32
align 16

extern grab_xmm0
grab_xmm0:
	mov eax, [esp+4]
	movdqu OWORD [eax], xmm0
	ret

extern grab_xmm1
grab_xmm1:
	mov eax, [esp+4]
	movdqu OWORD [eax], xmm1
	ret

extern grab_xmm2
grab_xmm2:
	mov eax, [esp+4]
	movdqu OWORD [eax], xmm2
	ret

extern grab_xmm3
grab_xmm3:
	mov eax, [esp+4]
	movdqu OWORD [eax], xmm3
	ret

extern grab_mxcsr
grab_mxcsr:
	sub esp, 4
	stmxcsr DWORD [esp]
	pop eax
	ret

extern grab_fsw
grab_fsw:
	xor eax, eax
	fstsw ax
	ret

extern geteip
geteip:
	pop eax
	jmp eax

extern asm_switch_stack
asm_switch_stack:
	ret

extern process_thumb
extern sse_level
process_thumb:
	cmp DWORD [sse_level], -1
	je .skip
	fxrstor [0x800]
	finit
	sub esp, 4
	mov DWORD [esp], 0x00001f80
	ldmxcsr DWORD [esp]
	add esp, 4
.skip:
	sti
	ret

extern yield
extern switch_task
yield:
	pushf
	cli
	pusha
	call switch_task
	popa
	popf
	ret

extern asm_fork_return
extern fork
extern multitasking_fork
extern lprintf
fork:
	pusha
	push esp
	call multitasking_fork
	add esp, 4
asm_fork_return:
	pop edi
	pop esi
	pop ebp
	add esp, 4
	pop ebx
	pop edx
	pop ecx
	add esp, 4
	ret

extern asm_task_return
asm_task_return:

extern asm_task_switch
asm_task_switch:
	mov ecx, [esp+4]
	mov ebp, [esp+8]
	mov esp, [esp+12]
	xor eax, eax
	jmp ecx
