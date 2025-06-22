bits 16
extern test_boot_worked
core_bootup:
	cli
	cld
	mov ax, 1
	mov WORD [0x504], ax
.loop:
	jmp .loop