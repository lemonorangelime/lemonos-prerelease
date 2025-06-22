#include <boot/initrd.h>
#include <string.h>
#include <multiboot.h>

multiboot_header_t * multiboot_header = (multiboot_header_t *) 0x00008000;

// stuff

int parse_multiboot(uint32_t eax, uint32_t ebx) {
	if (ebx == 0x8000) {
		initrd_module = (void *) multiboot_header->modules_address;
		return eax == 0x2BADB002;
	}
	void * p = (void *) ebx;
	memcpy(multiboot_header, p, sizeof(multiboot_header_t));
	initrd_module = (void *) multiboot_header->modules_address;
	return eax == 0x2BADB002;
}
