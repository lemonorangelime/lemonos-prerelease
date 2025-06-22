bits 32
extern get_apic_id
get_apic_id:
	push ebx
	push ecx
	push edx
	mov eax, 0x01
	cpuid
	shr ebx, 24
	mov eax, ebx
	pop edx
	pop ecx
	pop ebx
	ret