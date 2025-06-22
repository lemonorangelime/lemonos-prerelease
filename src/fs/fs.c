// we need to port the vfs here so LemonOS can finally acutlaly do something with it's drives

#include <stdint.h>
#include <multitasking.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fs/drive.h>
#include <fs/fs.h>

int fs_is_separator(char chr) {
	return chr == '/' || chr == '\\' || chr == '>';
}

char * fs_skip_separators(char * path) {
	while (fs_is_separator(*path) && *path) {
		path++;
	}
	return path;
}

char * fs_next_token(char * token) {
	while ((!fs_is_separator(*token)) && *token) {
		token++;
	}
	return token;
}

int fs_is_last_token(char * token) {
	token = fs_skip_separators(token);
	if (*token == 0) {
		return 1;
	}

	char * next = fs_next_token(token);
	return *next == 0;
}

char * fs_traverse_iterator(path_iterator_t * iterator) {
	char * token = fs_skip_separators(iterator->token);
	if (*token == 0) {
		return NULL;
	}
	iterator->token = fs_next_token(token);
	iterator->token_size = iterator->token - token;
	return token;
}

static int fd_size(int type) {
	switch (type) {
		case FD_SOCKET: return sizeof(fd_socket_t);
	}
	return -1;
}

fd_t * malloc_fd(int type) {
	int size = fd_size(type);
	if (size == -1) {
		return NULL;
	}
	process_t * process = *get_current_process();
	fd_t * fd = malloc(size);
	memset(fd, 0, size);
	fd->number = process->fd_top++;
	fd->type = type;
	if (!process->fds) {
		process->fds = malloc(sizeof(fd_list_t));
		memset(process->fds, 0, sizeof(fd_list_t));
	}
	process->fds->fds = linked_add(process->fds->fds, fd);
	return fd;
}

fd_t * create_fd(int type, fd_write_t write) {
	fd_t * fd = malloc_fd(type);
	if (!fd) {
		return NULL;
	}
	fd->write = write;
	return fd;
}

fd_t * find_fd(int number) {
	process_t * process = *get_current_process();
	if (!process->fds || !process->fds->fds) {
		return NULL;
	}
	linked_t * node = process->fds->fds;
	linked_iterator_t iterator = {node};
	while (node) {
		fd_t * fd = node->p;
		if (fd->number == number) {
			return fd;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}
