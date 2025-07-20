// we need to port the vfs here so LemonOS can finally acutlaly do something with it's drives

#include <stdint.h>
#include <multitasking.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fs/interfaces/fat32.h>
#include <fs/drive.h>
#include <fs/fs.h>

linked_t * device_nodes = NULL;
linked_t * fs_interfaces = NULL;
linked_t * filesystems = NULL;
fs_t * rootfs = NULL;

fs_node_t * fs_search_device(fs_node_t * node) {
	if (!node) {
		return NULL;
	}
	linked_iterator_t iterator = {device_nodes};
	linked_t * linked = linked_step_iterator(&iterator);
	while (linked) {
		fs_device_node_t * devicenode = linked->p;
		linked = linked_step_iterator(&iterator);
		if (devicenode->origin != node->fs) {
			continue;
		}
		if (devicenode->addr != node->addr) {
			continue;
		}
		return (fs_node_t *) devicenode;// fs_get_root(devicenode->fs, node);
	}
	return node;
}

void fs_attach_device(fs_node_t * node, fs_t * device) {
	if (!node) {
		return;
	}
	fs_device_node_t * devicenode = malloc(sizeof(fs_device_node_t));
	devicenode->addr = node->addr;
	devicenode->flags = node->flags;
	devicenode->fs = device;
	devicenode->origin = node->fs;
	device_nodes = linked_add(device_nodes, devicenode);
}

fs_node_t * fs_get_root(fs_t * fs, fs_node_t * node) {
	if (!node) {
		node = malloc(sizeof(fs_node_t));
		node->flags = FS_FLAGS_MALLOC;
		node->offset = 0;
		node = fs_get_root(fs, node);
	}
	node = fs->get_root(fs, node);
	node->fs = fs;
	return node;
}

fs_node_t * fs_search(fs_node_t * node, uint16_t * token, int len) {
	if (!node) {
		return NULL;
	}
	node = node->fs->search(node->fs, node, token, len);
	if (!node) {
		return NULL;
	}
	if ((node->flags & FS_FLAGS_DEVICE) == FS_FLAGS_DEVICE) {
		node = fs_search_device(node);
	}
	return node;
}

fs_node_t * fs_iterate(fs_node_t * node) {
	if (!node) {
		return NULL;
	}
	return node->fs->iterate(node->fs, node);
}

fs_node_t * fs_get_child(fs_node_t * node) {
	if (!node) {
		return NULL;
	}
	return node->fs->get_child(node->fs, node);
}

fs_node_t * fs_node_backtrack(fs_node_t * node) {
	if (!node) {
		return NULL;
	}
	return node->fs->backtrack(node->fs, node);
}

int fs_is_dotfile(uint16_t * token, int size) {
	if (size > 2) {
		return 0;
	}
	if (token[1] != '.' && size == 2) {
		return 0;
	}
	return token[0] == '.';
}

fs_node_t * fs_locate_dotfile(fs_node_t * node, uint16_t * token, int size) {
	switch (size) {
		default:
		case 1:
			return node;
		case 2:
			return fs_node_backtrack(node);
	}
}

fs_node_t * fs_path2node(fs_t * fs, uint16_t * path) {
	fs_node_t * node = malloc(sizeof(fs_node_t));
	node->flags = FS_FLAGS_MALLOC;
	node->offset = 0;
	node = fs_get_root(fs, node);

	path_iterator_t iterator = {path, 0};
	uint16_t * token = fs_step_iterator(&iterator);
	while (token && node) {
		int size = iterator.token_size;
		fs_node_t * old_node = node;
		if (fs_is_dotfile(token, size)) {
			node = fs_locate_dotfile(node, token, size);
			token = fs_step_iterator(&iterator);
			continue;
		}
		node = fs_search(node, token, size);
		token = fs_step_iterator(&iterator);
		if (!node) {
			free(old_node);
			return NULL;
		}
	}
	return node;
}

fs_node_t * fs_create(fs_node_t * directory, uint16_t * path, uint32_t flags) {
	fs_node_t * node = malloc(sizeof(fs_node_t));
	node->flags = FS_FLAGS_MALLOC;
	node->offset = 0;
	node->fs = directory->fs;
	node = fs_get_root(directory->fs, node);

	return directory->fs->create(directory->fs, directory, node, path, flags);
}

fs_node_t * fs_path_touch(fs_t * fs, uint16_t * path) {
	fs_node_t * node = malloc(sizeof(fs_node_t));
	node->flags = FS_FLAGS_MALLOC;
	node->offset = 0;
	node = fs_get_root(fs, node);

	fs_node_t * new = NULL;
	path_iterator_t iterator = {path, 0};
	uint16_t * token = fs_step_iterator(&iterator);
	while (token && node) {
		int size = iterator.token_size;
		uint16_t * old_token = token;
		fs_node_t * old_node = node;
		if (fs_is_dotfile(token, size)) {
			node = fs_locate_dotfile(node, token, size);
			token = fs_step_iterator(&iterator);
			continue;
		}
		token = fs_step_iterator(&iterator);
		if (!token) {
			uint16_t * copy = malloc((size * 2) + 1);
			for (int i = 0; i < size; i++) {
				copy[i] = old_token[i];
			}
			copy[size] = 0;
			new = fs_create(node, copy, 0);
			free(node);
			free(copy);
			return new;
		}
		node = fs_search(node, old_token, size);
	}
	free(node);
	return NULL;
}

uint16_t * fs_read_node_name(fs_node_t * node) {
	if (!node) {
		return NULL;
	}
	return node->fs->read_name(node->fs, node);
}

void fs_stat(fs_node_t * node, fs_statbuf_t * statbuf) {
	if (!node) {
		return;
	}
	return node->fs->stat(node->fs, node, statbuf);
}

size_t fs_read(fs_node_t * node, void * buffer, size_t size) {
	if (!node) {
		return -1;
	}
	return node->fs->read(node->fs, node, buffer, size);
}

size_t fs_write(fs_node_t * node, void * buffer, size_t size) {
	if (!node) {
		return -1;
	}
	return node->fs->write(node->fs, node, buffer, size);
}

void fs_set_flag(fs_node_t * node, uint32_t flags) {
	if (!node) {
		return;
	}
	node->fs->set_flag(node->fs, node, flags);
}

fs_interface_t * fs_create_interface() {
	return malloc(sizeof(fs_interface_t));
}

void fs_register_interface(fs_interface_t * interface) {
	fs_interfaces = linked_add(fs_interfaces, interface);
}

fs_t * fs_get_fs(int number) {
	linked_t * node = linked_get(filesystems, number);
	if (!node) {
		return NULL;
	}
	return node->p;
}




fs_node_t * mounter_search_node(fs_t * fs, fs_node_t * node, uint16_t * token, int len) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	memcpy(mounter->node_copy, mounter->node, sizeof(fs_node_t));
	return mounter->node->fs->search(mounter->node->fs, mounter->node_copy, token, len);
}

size_t mounter_write(fs_t * fs, fs_node_t * node, void * buffer, size_t len) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->write(mounter->node->fs, node, buffer, len);
}

size_t mounter_read(fs_t * fs, fs_node_t * node, void * buffer, size_t len) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->read(mounter->node->fs, node, buffer, len);
}

void mounter_stat(fs_t * fs, fs_node_t * node, fs_statbuf_t * statbuf) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->stat(mounter->node->fs, node, statbuf);
}

fs_node_t * mounter_get_root(fs_t * fs, fs_node_t * node) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->get_root(mounter->node->fs, node);
}

uint16_t * mounter_read_name(fs_t * fs, fs_node_t * node) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->read_name(mounter->node->fs, node);
}

fs_node_t * mounter_iterate(fs_t * fs, fs_node_t * node) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->iterate(mounter->node->fs, node);
}

fs_node_t * mounter_get_child(fs_t * fs, fs_node_t * node) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->get_child(mounter->node->fs, node);
}

fs_node_t * mounter_backtrack(fs_t * fs, fs_node_t * node) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->backtrack(mounter->node->fs, node);
}

fs_node_t * mounter_create(fs_t * fs, fs_node_t * directory, fs_node_t * node, uint16_t * name, uint32_t flags) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->create(mounter->node->fs, directory, node, name, flags);
}

void mounter_set_flag(fs_t * fs, fs_node_t * node, uint32_t flags) {
	fs_mounter_t * mounter = (fs_mounter_t *) fs;
	return mounter->node->fs->set_flag(mounter->node->fs, node, flags);
}



void fs_mount(fs_t * fs, uint16_t * path, fs_node_t * node) {
	fs_mounter_t * mounterfs = malloc(sizeof(fs_mounter_t));
	mounterfs->search = mounter_search_node;
	mounterfs->write = mounter_write;
	mounterfs->read = mounter_read;
	mounterfs->stat = mounter_stat;
	mounterfs->get_root = mounter_get_root;
	mounterfs->read_name = mounter_read_name;
	mounterfs->iterate = mounter_iterate;
	mounterfs->get_child = mounter_get_child;
	mounterfs->backtrack = mounter_backtrack;
	mounterfs->create = mounter_create;
	mounterfs->set_flag = mounter_set_flag;
	mounterfs->drive = fs->drive;
	mounterfs->node = node;
	mounterfs->node_copy = malloc(sizeof(fs_node_t));

	fs_node_t * mounternode = fs_path2node(fs, path);
	fs_attach_device(mounternode, (fs_t *) mounterfs);
}

void fs_mount_others(fs_t * rootfs) {
	linked_iterator_t iterator = {filesystems};
	linked_t * node = linked_step_iterator(&iterator);
	fs_node_t * devnode = fs_path2node(rootfs, u"/dev/");
	if (!devnode) {
		//return;
		fs_node_t rootnode;
		fs_node_t * root = fs_get_root(rootfs, &rootnode);
		devnode = fs_create(root, u"dev", STAT_FLAGS_DIRECTORY);
	}
	int i = 0;
	while (node) {
		fs_t * fs = node->p;
		uint16_t dev_template_abs[16] = u"/dev/disk0";
		uint16_t dev_template[16] = u"disk0";
		dev_template_abs[9] = i + '0';
		dev_template[4] = dev_template_abs[9];

		node = linked_step_iterator(&iterator);
		i++;

		if (!fs_search(devnode, dev_template, 5)) {
			fs_node_t * newnode = fs_create(devnode, dev_template, STAT_FLAGS_DEVICE | STAT_FLAGS_DIRECTORY);
			if (!newnode) {
				continue;
			}
			devnode = fs_path2node(rootfs, u"/dev/");
			free(newnode);
		}

		fs_mount(rootfs, dev_template_abs, fs_get_root(fs, NULL));
	}
}

int fs_detect_drive(linked_t * drive_node, void * pass) {
	drive_t * drive = drive_node->p;
	linked_iterator_t iterator = {fs_interfaces};
	linked_t * node = linked_step_iterator(&iterator);
	static int i = 0;
	while (node) {
		fs_interface_t * interface = node->p;
		if (interface->detect(drive)) {
			fs_t * fs = interface->create(drive);
			if (i++ == 0) {
				rootfs = fs;
			}
			filesystems = linked_add(filesystems, fs);
		}
		node = linked_step_iterator(&iterator);
	}
}

void fs_init() {
	fat_init();

	linked_iterate(drives, fs_detect_drive, NULL);

	if (!rootfs) {
		return;
	}

	fs_mount_others(rootfs);
}


int fs_is_separator(uint16_t chr) {
	return chr == u'/' || chr == u'\\' || chr == u'>';
}

uint16_t * fs_skip_separators(uint16_t * path) {
	while (fs_is_separator(*path) && *path) {
		path++;
	}
	return path;
}

uint16_t * fs_next_token(uint16_t * token) {
	while ((!fs_is_separator(*token)) && *token) {
		token++;
	}
	return token;
}

int fs_is_last_token(uint16_t * token) {
	token = fs_skip_separators(token);
	if (*token == 0) {
		return 1;
	}

	uint16_t * next = fs_next_token(token);
	return *next == 0;
}

uint16_t * fs_step_iterator(path_iterator_t * iterator) {
	uint16_t * token = fs_skip_separators(iterator->token);
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
		case FD_FILE: return sizeof(fd_file_t);
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

fd_t * create_fd(int type, fd_write_t write, fd_read_t read) {
	fd_t * fd = malloc_fd(type);
	if (!fd) {
		return NULL;
	}
	fd->write = write;
	fd->read = read;
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
