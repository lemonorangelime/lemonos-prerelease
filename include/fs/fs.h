#pragma once

#include <stdint.h>
#include <net/socket.h>

typedef struct fs_filesystem fs_filesystem_t;
typedef struct fs_node fs_node_t;
typedef struct fd fd_t;

typedef int (* fd_write_t)(fd_t * fd, void * buffer, uint32_t size);

typedef struct fs_node {
	int _empty;
} fs_node_t;

typedef struct fs_filesystem {
	void * priv;
} fs_filesystem_t;

typedef struct {
	char * token;
	int token_size;
} path_iterator_t;

typedef struct fd {
	int number;
	int type;
	fd_write_t write;
} fd_t;

typedef struct {
	int number;
	int type;
	fd_write_t write;
	int protocol;
	int timeout;
	uint32_t flags;
	socket_t * socket;
	socket_client_t * client;
} fd_socket_t;

typedef struct {
	linked_t * fds;
} fd_list_t;

enum {
	FD_VIRTUAL,
	FD_FILE,
	FD_SOCKET,
};

enum {
	FD_SOCKET_FLAG_SERVER    = 0b00000001,
	FD_SOCKET_FLAG_CONNECTED = 0b00000010,
};

enum {
	FORMAT_UNFORMATTED,
	FORMAT_RAW,
	FORMAT_FAT12,
	FORMAT_FAT32,
	FORMAT_EXT,
	FORMAT_EXT2,
	FORMAT_EXT3,
	FORMAT_EXT4,
};

enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

int fs_is_separator(char chr);
char * fs_skip_separators(char * path);
char * fs_next_token(char * token);
int fs_is_last_token(char * token);
char * fs_traverse_iterator(path_iterator_t * iterator);
fd_t * create_fd(int type, fd_write_t write);
fd_t * find_fd(int fd);
