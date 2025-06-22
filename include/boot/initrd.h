#pragma once

#include <multiboot.h>

#define DISK_MAX 32

typedef struct {
        char filename[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char typeflag[1];
} tar_header_t;

typedef struct {
	tar_header_t * file;
	uint32_t head;
} initrd_fd_t;

extern tar_header_t * initrd[DISK_MAX];
extern multiboot_module_t * initrd_module;

initrd_fd_t * initrd_open(char * filename);
void initrd_seek(initrd_fd_t * fd, int offset, int whence);
uint32_t initrd_tell(initrd_fd_t * fd);
uint32_t initrd_size(initrd_fd_t * fd);
uint32_t initrd_read(void * buffer, size_t length, initrd_fd_t * fd);
void initrd_init();