#include <boot/initrd.h>
#include <string.h>
#include <fs/fs.h>
#include <memory.h>
#include <multiboot.h>

tar_header_t * initrd[DISK_MAX];
multiboot_module_t * initrd_module;
int initrd_length = 0;

static uint32_t tar_size(const char * in) {
	uint32_t size = 0;
	uint32_t count = 1;
	for (uint32_t j = 11; j > 0; j--, count *= 8) {
		size += ((in[j - 1] - '0') * count);
	}
	return size;
}

tar_header_t * initrd_get_header(char * filename) {
	for (int i = 0; i < initrd_length; i++) {
		tar_header_t * head = initrd[i];
		if (strcmp(head->filename, filename) == 0) {
			return head;
		}
	}
	return NULL;
}

initrd_fd_t * initrd_open(char * filename) {
	tar_header_t * head = initrd_get_header(filename);
	initrd_fd_t * fd;
	if (!head) {
		return NULL;
	}
	fd = malloc(sizeof(initrd_fd_t));
	fd->file = head;
	fd->head = 0;
	return fd;
}

void initrd_seek(initrd_fd_t * fd, int offset, int whence) {
	switch (whence) {
		case SEEK_SET:
			fd->head = offset;
			return;
		case SEEK_CUR:
			fd->head += offset;
			return;
		case SEEK_END:
			uint32_t end = tar_size(fd->file->size);
			fd->head = end + offset;
			return;
	}
}

uint32_t initrd_tell(initrd_fd_t * fd) {
	return fd->head;
}

uint32_t initrd_size(initrd_fd_t * fd) {
	return tar_size(fd->file->size);
}

uint32_t initrd_read(void * buffer, size_t length, initrd_fd_t * fd) {
	uint32_t end = tar_size(fd->file->size);
	uint8_t * data = (((void *) fd->file) + 512) + fd->head;
	uint8_t * buff = buffer;

	size_t i = 0;
	while (i < length) {
		if (fd->head > end) {
			return i;
		}
		*buff++ = *data++;
		fd->head++;
		i++;
	}
	return i;
}

void initrd_init() {
	void * p = initrd_module->start;
	uint32_t size;
	int i = 0;
	for (; i < DISK_MAX; i++) {
		tar_header_t * head = p;
		if (*head->filename == 0) {
			break;
		}
		size = tar_size(head->size);
		initrd[i] = head;
		p += ((size / 512) + 1) * 512;
		if (size % 512) {
			p += 512;
		}
	}

	initrd_length = i;
}
