#include <boot/initrd.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <input/input.h>
#include <multitasking.h>
#include <bin_formats/elf.h>

int elf_load(char * filename, void * buffer, int argc, char * argv[]) {
	elf_t * elf = buffer;
	elf_ph_t * ph = buffer + elf->ph_off;
	uint16_t * name = malloc((strlen(filename) * 2) + 2);
	process_t * proc;
	void * entry;
	if (elf->type != ET_EXEC && elf->type != ET_DYN && elf->type != ET_REL) {
		return 0;
	}

	if (elf->machine != ET_UNSPECIFIED && elf->machine != ET_I386) {
		return 0;
	}

	if (elf->abi != ABI_UNIX) {
		return 0;
	}

	atoustr(name, filename);
	entry = buffer + elf->entry + ph->offset;
	proc = create_process_paused(name, entry);
	process_push_args(proc, argc, argv);
	multitasking_track_allocation(proc, buffer); // need this freed but after the program exits
	proc->killed = 1;
	return proc->pid;
}

// execute from the ram disk
// !! THIS IS EVIL !!
int exec_initrd(char * filename, int argc, char * argv[]) {
	initrd_fd_t * fd = initrd_open(filename);
	if (!fd) {
		return 0;
	}

	size_t size = initrd_size(fd);
	void * buffer = malloc(size);
	uint32_t * magic = buffer;
	initrd_read(buffer, 4, fd);

	if (*magic != 0x464c457f) {
		free(buffer);
		free(fd);
		return 0;
	}
	initrd_read(buffer + 4, size - 4, fd);

	int r = elf_load(filename, buffer, argc, argv);
	free(fd);
	return r;
}
