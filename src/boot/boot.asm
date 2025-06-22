bits 32
align 4
section .multiboot
	dd 0x1badb002
	dd 0x07
	dd -(0x1badb002 + 0x07)
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 480
	dd 640
	dd 32

section .stack
stack_bottom:
times 65535 db 0  ; 64kb
stack_top:

section .text

align 16

global start
extern main
extern sse_init
extern avx_init
extern fpu_init
extern mce_init
extern cache_init
extern tce_init
extern v8086_init
start:
	cli
	clts
	mov esp, stack_top
	pusha
	call fpu_init
	call sse_init
	call mce_init
	call tce_init
	call v8086_init
	; call cache_init (need this? it causes problems on some hardware)
	call avx_init
	popa
	mov esp, stack_top
	push ebx
	push eax
	call main
	cli
	hlt
