#pragma once

#include <stdint.h>
#include <net/socket.h>
#include <fs/drive.h>

typedef struct fs fs_t;
typedef struct fs_node fs_node_t;
typedef struct statbuf fs_statbuf_t;
typedef struct fd fd_t;

typedef int (* fd_write_t)(fd_t * fd, void * buffer, uint32_t size);
typedef int (* fd_read_t)(fd_t * fd, void * buffer, uint32_t size);

typedef fs_node_t * (* fs_search_node_t)(fs_t * fs, fs_node_t * node, uint16_t * token, int len);
typedef size_t (* fs_write_t)(fs_t * fs, fs_node_t * node, void * buffer, size_t len);
typedef size_t (* fs_read_t)(fs_t * fs, fs_node_t * node, void * buffer, size_t len);
typedef void (* fs_stat_t)(fs_t * fs, fs_node_t * node, fs_statbuf_t * statbuf);
typedef fs_node_t * (* fs_get_root_t)(fs_t * fs, fs_node_t * node);
typedef uint16_t * (* fs_read_name_t)(fs_t * fs, fs_node_t * node);
typedef fs_node_t * (* fs_get_child_t)(fs_t * fs, fs_node_t * node);
typedef fs_node_t * (* fs_iterate_t)(fs_t * fs, fs_node_t * node);
typedef fs_node_t * (* fs_backtrack_t)(fs_t * fs, fs_node_t * node);
typedef fs_node_t * (* fs_create_t)(fs_t * fs, fs_node_t * directory, fs_node_t * node, uint16_t * name, uint32_t flags);
typedef void (* fs_set_flag_t)(fs_t * fs, fs_node_t * node, uint32_t bits);
typedef int (* fs_interface_detect_t)(drive_t * disk);
typedef fs_t * (* fs_interface_create_t)(drive_t * disk);

typedef struct fs {
	fs_search_node_t search;
	fs_write_t write;
	fs_read_t read;
	fs_stat_t stat;
	fs_get_root_t get_root;
	fs_read_name_t read_name;
	fs_get_child_t get_child;
	fs_iterate_t iterate;
	fs_backtrack_t backtrack;
	fs_create_t create;
	fs_set_flag_t set_flag;
	drive_t * drive;
} fs_t;

typedef struct {
	fs_search_node_t search;
	fs_write_t write;
	fs_read_t read;
	fs_stat_t stat;
	fs_get_root_t get_root;
	fs_read_name_t read_name;
	fs_get_child_t get_child;
	fs_iterate_t iterate;
	fs_backtrack_t backtrack;
	fs_create_t create;
	fs_set_flag_t set_flag;
	drive_t * drive;
	fs_node_t * node;
	fs_node_t * node_copy;
} fs_mounter_t;

typedef struct {
	fs_interface_detect_t detect;
	fs_interface_create_t create;
} fs_interface_t;

typedef struct fs_node {
	uint64_t addr;
	uint32_t flags;
	uint32_t offset;
	fs_t * fs;
} fs_node_t;

typedef struct {
	uint64_t addr;
	uint32_t flags;
	uint32_t offset;
	fs_t * fs;
	fs_t * origin;
} fs_device_node_t;

typedef struct statbuf {
	size_t size;
	uint32_t flags;
} fs_statbuf_t;




typedef struct {
	uint16_t * token;
	int token_size;
} path_iterator_t;

typedef struct fd {
	int number;
	int type;
	fd_write_t write;
	fd_read_t read;
} fd_t;

typedef struct {
	int number;
	int type;
	fd_write_t write;
	fd_read_t read;
	fs_node_t * node;
} fd_file_t;

typedef struct {
	int number;
	int type;
	fd_write_t write;
	fd_read_t read;
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
	FS_TYPE_DIRECTORY,
	FS_TYPE_FILE,
	FS_TYPE_DEVICE,
	FS_TYPE_MOUNT,
};

enum {
	FS_FLAGS_ROOT      = 0b0000000000000001,
	FS_FLAGS_DEVICE    = 0b0000000000000010,
	FS_FLAGS_MALLOC    = 0b0000000100000000,
};

enum {
	STAT_FLAGS_RDONLY    = 0b0000000000000001,
	STAT_FLAGS_DIRECTORY = 0b0000000000000010,
	STAT_FLAGS_DEVICE    = 0b0000000000000100,
};


enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

extern fs_t * rootfs;

fs_node_t * fs_search_device(fs_node_t * node);
void fs_attach_device(fs_node_t * node, fs_t * device);
fs_node_t * fs_get_root(fs_t * fs, fs_node_t * node);
fs_node_t * fs_search(fs_node_t * node, uint16_t * token, int len);
fs_node_t * fs_iterate(fs_node_t * node);
fs_node_t * fs_get_child(fs_node_t * node);
fs_node_t * fs_node_backtrack(fs_node_t * node);
int fs_is_dotfile(uint16_t * token, int size);
fs_node_t * fs_locate_dotfile(fs_node_t * node, uint16_t * token, int size);
fs_node_t * fs_path2node(fs_t * fs, uint16_t * path);
fs_node_t * fs_path_touch(fs_t * fs, uint16_t * path);
fs_node_t * fs_create(fs_node_t * directory, uint16_t * path, uint32_t flags);
uint16_t * fs_read_node_name(fs_node_t * node);
void fs_stat(fs_node_t * node, fs_statbuf_t * statbuf);
size_t fs_read(fs_node_t * node, void * buffer, size_t size);
size_t fs_write(fs_node_t * node, void * buffer, size_t size);
void fs_set_flag(fs_node_t * node, uint32_t flags);
fs_interface_t * fs_create_interface();
void fs_register_interface(fs_interface_t * interface);
fs_t * fs_get_fs(int number);
void fs_mount_others(fs_t * rootfs);
void fs_init();

int fs_is_separator(uint16_t chr);
uint16_t * fs_skip_separators(uint16_t * path);
uint16_t * fs_next_token(uint16_t * token);
int fs_is_last_token(uint16_t * token);
uint16_t * fs_step_iterator(path_iterator_t * iterator);

fd_t * find_fd(int number);
fd_t * create_fd(int type, fd_write_t write, fd_read_t read);
fd_t * malloc_fd(int type);