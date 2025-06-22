bits 32
align 16
extern hlt
extern halt
extern sleep
extern disable_interrupts
extern enable_interrupts
extern amd_reboot
amd_reboot: ; reboot a amd cpu (is this good idea?)
	push edx
	push ecx
	mov ecx, 0x1a2
	rdmsr
	wrmsr
	xor eax, eax
	pop ecx
	pop edx
	ret	

hlt: ; a single halt
	hlt
	ret

halt: ; halt forever
	cli
.loop:
	hlt ; we can even enter a deep C state if we want
	jmp .loop

sleep: ; also halt forever, but allow interrupts while we are napping
.loop:
	hlt ; for this we cant enter a C state deeper than C1 (hlt)
	jmp .loop

disable_interrupts:
	cli
	ret

enable_interrupts:
	sti
	ret