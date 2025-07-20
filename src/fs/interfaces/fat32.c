#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <fs/drive.h>
#include <fs/fs.h>
#include <fs/interfaces/fat32.h>

int fat_size = 32;

uint32_t fat_sector_count(fat32_t * header) {
	if (header->total_sectors == 0) {
		return header->large_sectors;
	}
	return header->total_sectors;
}

int fat_guess_size(fat32_t * header) {
	uint32_t clusters = fat_sector_count(header) / header->cluster_size;
	if (clusters < 4085) {
		return 12;
	}
	if (clusters < 65525) {
		return 16;
	}
	return 32;
}

uint32_t fat_sectors_per_fat(fat32_t * header) {
	if (fat_size == 32) {
		return header->sector_per_fat32;
	}
	return header->sector_per_fat;
}

uint64_t fat_clust2addr(fat32_t * header, uint64_t cluster) {
	uint64_t start = (header->reserved_sectors * 512) + header->tables * (fat_sectors_per_fat(header) * 512);
	uint64_t clust_size = (header->cluster_size * 512);
	return start + ((cluster - 2) * clust_size);
}

uint64_t fat_addr2clust(fat32_t * header, uint64_t addr) {
	uint64_t start = (header->reserved_sectors * 512) + header->tables * (fat_sectors_per_fat(header) * 512);
	uint64_t clust_size = (header->cluster_size * 512);
	return ((addr - start) / clust_size) + 2;
}

uint64_t fat_indexinto(fat32_t * header, uint64_t addr) {
	uint64_t cluster = fat_addr2clust(header, addr);
	return addr - fat_clust2addr(header, cluster);
}

int fat_directory_name_len(fat32_directory_t * directory) {
	if ((directory->attributes & FAT_ATTRIBUTES_LONG) != FAT_ATTRIBUTES_LONG) {
		return 12;
	}

	char buffer[28];
	memset(buffer, 0, 28);

	int i = 0;
	fat32_long_name_t * name = (fat32_long_name_t *) (directory);
	while (name->attributes == FAT_ATTRIBUTES_LONG) {
		memcpy(buffer, name->first_chunk, 10);
		memcpy(buffer + 10, name->second_chunk, 12);
		memcpy(buffer + 22, name->third_chunk, 4);
		i += ustrlen((uint16_t *) buffer);
		name++;
	}
	return i;
}

void fat_directory_read_name(fat32_directory_t * directory, uint16_t * out) {
	if ((directory->attributes & FAT_ATTRIBUTES_LONG) != FAT_ATTRIBUTES_LONG) {
		for (int i = 0; i < 8; i++) {
			out[i] = directory->filename[i];
		}
		out[8] = '.';
		for (int i = 8; i < 11; i++) {
			out[i + 1] = directory->filename[i];
		}
		return;
	}

	char buffer[28];
	memset(buffer, 0, 28);
	fat32_long_name_t * name = (fat32_long_name_t *) (directory);
	while (name->attributes == FAT_ATTRIBUTES_LONG) {
		uint8_t order = (name->order & 0x0f) - 1;
		memcpy(buffer, name->first_chunk, 10);
		memcpy(buffer + 10, name->second_chunk, 12);
		memcpy(buffer + 22, name->third_chunk, 4);

		memcpy(out + (order * 13), buffer, ustrlen((uint16_t *) buffer) * 2);

		name++;
	}
}

fat32_directory_t * fat_directory_skip_name(fat32_directory_t * directory) {
	if ((directory->attributes & FAT_ATTRIBUTES_LONG) != FAT_ATTRIBUTES_LONG) {
		return directory;
	}

	fat32_long_name_t * name = (fat32_long_name_t *) (directory);
	while (name->attributes == FAT_ATTRIBUTES_LONG) {
		name++;
	}
	return (fat32_directory_t *) name;
}

uint32_t fat_get_cluster_info(fat32_t * header,  drive_t * disk, uint32_t cluster) {
	char fat_table[512];
	uint64_t offset = cluster * 4;
	uint64_t entry = offset % 512;
	drive_read_linear(disk, (header->reserved_sectors + (offset >> 9)) * 512, fat_table, 512);

	return (*(uint32_t *) &fat_table[entry]) & 0x0fffffff;
}

uint32_t fat_get_cluster_info_cached(fat32_t * header,  drive_t * disk, uint32_t cluster, uint32_t * state, char * cache) {
	uint64_t offset = cluster * 4;
	uint64_t entry = offset % 512;
	uint32_t fat_sector = offset >> 9;
	if (*state != fat_sector) {
		drive_read_linear(disk, (header->reserved_sectors + fat_sector) * 512, cache, 512);
		*state = fat_sector;
		//printf(u"load new fat %r\n", fat_sector);
	}

	return (*(uint32_t *) &cache[entry]) & 0x0fffffff;
}

void fat_link_cluster(fat32_t * header,  drive_t * disk, uint32_t cluster, uint32_t cluster2) {
	char fat_table[512];
	uint64_t offset = cluster * 4;
	uint64_t entry = offset % 512;
	drive_read_linear(disk, (header->reserved_sectors + (offset >> 9)) * 512, fat_table, 512);

	uint32_t * entryp = (uint32_t *) &fat_table[entry];
	*entryp = cluster2;
	drive_write_linear(disk, (header->reserved_sectors + (offset >> 9)) * 512, fat_table, 512);
	//disk->write_linear(fat_table, (fat_sectors_per_fat(header) + header->reserved_sectors + (offset >> 9)) * 512, 512);
}

uint32_t fat_alloc_cluster(fat32_t * header,  drive_t * disk) {
	char fat_table[512];
	uint64_t offset = 0x30;
	uint64_t entry = offset % 512;
	drive_read_linear(disk, (header->reserved_sectors + (offset >> 9)) * 512, fat_table, 512);

	uint64_t max = fat_sectors_per_fat(header) * 128;
	while (offset < max) {
		entry = offset % 512;
		if (entry == 0) {
			drive_read_linear(disk, (header->reserved_sectors + (offset >> 9)) * 512, fat_table, 512);
		}
		uint32_t * cluster = (uint32_t *) &fat_table[entry];
		if (*cluster == 0) {
			*cluster = 0x0fffffff;
			drive_write_linear(disk, (header->reserved_sectors + (offset >> 9)) * 512, fat_table, 512);
			//disk->write_linear(fat_table, (fat_sectors_per_fat(header) + header->reserved_sectors + (offset >> 9)) * 512, 512);
			return offset / 4;
		}

		offset += 4;
	}
	return 0x00000000;
}

void fat_read_next(fat32_t * header, drive_t * disk, uint32_t cluster, char * buffer) {
	uint32_t next = fat_get_cluster_info(header, disk, cluster);
	if (next >= 0x0fffff00) {
			return;
	}
	uint64_t addr = fat_clust2addr(header, next);
	drive_read_linear(disk, addr, buffer + 512, 512);
}

void fat_write_cluster(fat32_t * header, drive_t * disk, uint32_t cluster, char * buffer) {
	uint64_t addr = fat_clust2addr(header, cluster);
	drive_write_linear(disk, addr, buffer, 512);

	//addr = fat_clust2addr(header, cluster);
	//drive_write_linear(disk, addr, buffer, 512);
}

void fat_write_next(fat32_t * header, drive_t * disk, uint32_t cluster, char * buffer) {
	uint32_t next = fat_get_cluster_info(header, disk, cluster);
	if (next >= 0x0fffff00) {
		return;
	}
	uint64_t addr = fat_clust2addr(header, next);
	drive_write_linear(disk, addr, buffer + 512, 512);
}

uint32_t fat_swap_next(fat32_t * header, drive_t * disk, uint32_t cluster, char * buffer) {
	uint32_t next = fat_get_cluster_info(header, disk, cluster);
	if (next >= 0x0fffff00) {
		return next;
	}
	uint64_t addr = fat_clust2addr(header, next);
	drive_read_linear(disk, addr, buffer, 512);
	return next;
}

fat32_directory_t * fat_next_entry(fat32_t * header, drive_t * disk, fat32_directory_t * directory, uint32_t * cluster, char * buffer) {
	fat32_long_name_t * name = (fat32_long_name_t *) (directory);
	while (name->attributes == FAT_ATTRIBUTES_LONG) {
		name++;
	}
	name++;
	if (name->order == 0) {
		return NULL;
	}

	if ((((char *) name) - buffer) >= 512) {
		uint32_t next = fat_get_cluster_info(header, disk, *cluster);
		uint64_t addr = fat_clust2addr(header, next);
		drive_read_linear(disk, addr, buffer, 512);
		fat_read_next(header, disk, next, buffer);
		*cluster = next;
		return (fat32_directory_t *) (((char *) name) - 512);
	}

	return (fat32_directory_t *) name;
}

fat32_directory_t * fat_next_entry_unsafe(fat32_t * header, drive_t * disk, fat32_directory_t * directory, uint32_t * cluster, char * buffer) {
	fat32_long_name_t * name = (fat32_long_name_t *) (directory);
	while (name->attributes == FAT_ATTRIBUTES_LONG) {
		name++;
	}
	name++;
	if ((((char *) name) - buffer) >= 512) {
		uint32_t next = fat_get_cluster_info(header, disk, *cluster);
		uint64_t addr = fat_clust2addr(header, next);
		drive_read_linear(disk, addr, buffer, 512);
		fat_read_next(header, disk, next, buffer);
		*cluster = next;
		return (fat32_directory_t *) (((char *) name) - 512);
	}

	return (fat32_directory_t *) name;
}

void fat_read_node_clusters(fat32_t * header, fs_node_t * node, char * buffer) {
	uint32_t cluster = fat_addr2clust(header, node->addr);
	uint64_t addr = fat_clust2addr(header, cluster);
	drive_t * disk = node->fs->drive;
	drive_read_linear(disk, addr, buffer, 512);
	fat_read_next(header, disk, cluster, buffer);
}

void fat_write_node_clusters(fat32_t * header, fs_node_t * node, char * buffer) {
	uint32_t cluster = fat_addr2clust(header, node->addr);
	uint64_t addr = fat_clust2addr(header, cluster);
	drive_t * disk = node->fs->drive;
	drive_write_linear(disk, addr, buffer, 512);
	fat_write_next(header, disk, cluster, buffer);
}

fs_node_t * fat_search_node(fs_t * fs, fs_node_t * node, uint16_t * token, int len) {
	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	uint32_t cluster = fat_addr2clust(header, node->addr);
	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	if ((node->flags & FS_FLAGS_ROOT) == 0) {
		drive_read_linear(disk, node->addr, buffer, 512);
		directory = (fat32_directory_t *) buffer;

		fat32_directory_t * file = fat_directory_skip_name(directory);
		if ((file->attributes & FAT_ATTRIBUTES_DIRECTORY) == 0) {
			return NULL;
		}

		cluster = file->root_cluster_low | (file->root_cluster_high << 16);
		drive_read_linear(disk, fat_clust2addr(header, cluster), directory, 512);
		fat_read_next(header, disk, cluster, buffer);
	}

	while (directory) {
		fat32_directory_t * file = fat_directory_skip_name(directory);
		if (file->filename[0] == 0x2e || file->filename[0] == 0xe5 || file->filename[0] == 0xffffffe5) {
			directory = fat_next_entry(header, disk, directory, &cluster, buffer);
			continue;
		}

		int name_len = fat_directory_name_len(directory);
		if (name_len != len) {
			directory = fat_next_entry(header, disk, directory, &cluster, buffer);
			continue;
		}
		uint16_t * name = malloc((name_len + 1) * 2);
		fat_directory_read_name(directory, name);
		if (memcmp(name, token, len * 2) == 0) {
			node->addr = fat_clust2addr(header, cluster) + (((char *) directory) - buffer);
			node->flags ^= node->flags & FS_FLAGS_ROOT;
			node->flags ^= node->flags & FS_FLAGS_DEVICE;
			node->flags |= (file->attributes & FAT_ATTRIBUTES_DEVICE) ? FS_FLAGS_DEVICE : 0;
			node->offset = 0;
			free(name);
			return node;
		}

		directory = fat_next_entry(header, disk, directory, &cluster, buffer);
		free(name);
	}
	return NULL;
}

size_t fat_write(fs_t * fs, fs_node_t * node, void * input, size_t len) {
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		return 0;
	}

	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	fat32_directory_t * file = fat_directory_skip_name(directory);
	if (file->attributes & FAT_ATTRIBUTES_DIRECTORY) {
		return 0;
	}

	uint32_t size = file->size;
	uint32_t cluster = file->root_cluster_low | (file->root_cluster_high << 16);
	if (cluster >= 0x0fffff00 || cluster == 0) {
		uint32_t newcluster = fat_alloc_cluster(header, disk);
		file->root_cluster_low = newcluster & 0xffff;
		file->root_cluster_high = (newcluster >> 16) & 0xffff;
		fat_write_node_clusters(header, node, buffer);
		cluster = newcluster;
	}

	char cache[512];
	size_t written = 0;
	uint32_t state = 0;
	uint32_t offset = node->offset;
	drive_read_linear(disk, header->reserved_sectors * 512, cache, 512);
	while (offset >= 512) {
		cluster = fat_get_cluster_info_cached(header, disk, cluster, &state, cache);
		offset -= 512;
		size -= 512;
	}

	uint64_t addr = fat_clust2addr(header, cluster);
	drive_read_linear(disk, addr, buffer, 512);
	if (offset != 0) {
		uint32_t fit = 512 - offset;
		if (fit > len) {
			fit = len;
		}
		drive_write_linear(disk, fat_clust2addr(header, cluster) + offset, input, fit);

		uint32_t nextcluster = fat_get_cluster_info_cached(header, disk, cluster, &state, cache);
		if (nextcluster >= 0x0fffff00) {
			uint32_t newcluster = fat_alloc_cluster(header, disk);
			fat_link_cluster(header, disk, cluster, newcluster);
			nextcluster = newcluster;
		}
		cluster = nextcluster;

		written += fit;
		input += fit;
		len -= fit;
		offset = 0;
	}

	while (len >= 512) {
		drive_write(disk, fat_clust2addr(header, cluster) >> 9, input, 512);
		input += 512;
		written += 512;
		len -= 512;

		uint32_t nextcluster = fat_get_cluster_info_cached(header, disk, cluster, &state, cache);
		if (nextcluster >= 0x0fffff00) {
			uint32_t newcluster = fat_alloc_cluster(header, disk);
			fat_link_cluster(header, disk, cluster, newcluster);
			nextcluster = newcluster;
		}
		cluster = nextcluster;
	}
	drive_read(disk, fat_clust2addr(header, cluster) >> 9, buffer, 512);
	if (len != 0) {
		memcpy(buffer, input, len);
		input += len;
		written += len;
		len -= len;
	}
	drive_write(disk, fat_clust2addr(header, cluster) >> 9, buffer, 512);

	cluster = fat_get_cluster_info_cached(header, disk, cluster, &state, cache);
	while (cluster < 0x0fffff00) {
		uint32_t nextcluster = fat_get_cluster_info_cached(header, disk, cluster, &state, cache);
		fat_link_cluster(header, disk, cluster, 0);
		cluster = nextcluster;
	}

	fat_read_node_clusters(header, node, buffer);
	file->size = written;
	fat_write_node_clusters(header, node, buffer);
	return written;
}

size_t fat_read(fs_t * fs, fs_node_t * node, void * output, size_t len) {
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		return 0;
	}

	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	fat32_directory_t * file = fat_directory_skip_name(directory);
	if (file->attributes & FAT_ATTRIBUTES_DIRECTORY) {
		return 0;
	}

	uint32_t size = file->size;
	uint32_t cluster = file->root_cluster_low | (file->root_cluster_high << 16);
	uint32_t offset = node->offset;
	size_t read = 0;
	if (offset >= size) {
		return 0;
	}
	drive_read_linear(disk, header->reserved_sectors * 512, buffer, 512);

	/* seek phase */
	uint32_t state = 0;
	while (offset >= 512) {
		cluster = fat_get_cluster_info_cached(header, disk, cluster, &state, buffer);
		offset -= 512;
		size -= 512;
	}

	uint64_t addr = fat_clust2addr(header, cluster);
	drive_read_linear(disk, addr, buffer, 512);
	if (offset != 0) {
		uint32_t fit = 512 - offset;
		if (fit > len) {
			fit = len;
		}
		if (fit > size) {
			fit = size;
		}
		memcpy(output, buffer + offset, fit);
		cluster = fat_swap_next(header, disk, cluster, buffer);
		read += fit;
		output += fit;
		len -= fit;
		size -= fit;
		offset = 0;
	}

	while (cluster < 0x0fffff00 && len > 0) {
		if (len > 512 && size > len) {
			memcpy(output, buffer, 512);
			output += 512;
			read += 512;
			len -= 512;
			size -= 512;
		} else if (size >= len) {
			memcpy(output, buffer, len);
			output += len;
			read += len;
			len -= len;
			size -= len;
			break;
		} else {
			memcpy(output, buffer, size);
			output += size;
			read += size;
			len -= size;
			size -= size;
			break;
		}
		cluster = fat_swap_next(header, disk, cluster, buffer);
	}
	node->offset += read;
	return read;
}

uint16_t * fat_read_name(fs_t * fs, fs_node_t * node) {
	drive_t * disk = fs->drive;
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		return NULL;
	}

	fat32_t * header = (fat32_t *) disk->sector1_cache;
	uint32_t cluster = fat_addr2clust(header, node->addr);
	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));

	int len = fat_directory_name_len(directory);
	uint16_t * name = malloc((len + 1) * 2);
	fat_directory_read_name(directory, name);
	name[len] = 0;
	return name;
}

void fat_stat(fs_t * fs, fs_node_t * node, fs_statbuf_t * statbuf) {
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		statbuf->size = 0;
		statbuf->flags = STAT_FLAGS_DIRECTORY;
		return;
	}
	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	uint32_t cluster = fat_addr2clust(header, node->addr);
	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	fat32_directory_t * file = fat_directory_skip_name(directory);

	uint32_t flags = 0;
	flags |= (file->attributes & FAT_ATTRIBUTES_RDONLY) ? STAT_FLAGS_RDONLY : 0;
	flags |= (file->attributes & FAT_ATTRIBUTES_DIRECTORY) ? STAT_FLAGS_DIRECTORY : 0;
	flags |= (file->attributes & FAT_ATTRIBUTES_DEVICE) ? STAT_FLAGS_DEVICE : 0;

	statbuf->size = file->size;
	statbuf->flags = flags;
}

fs_node_t * fat_iterate(fs_t * fs, fs_node_t * node) {
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		free(node);
		return NULL;
	}
	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	uint32_t cluster = fat_addr2clust(header, node->addr);
	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	directory = fat_next_entry(header, disk, directory, &cluster, buffer);
	if (!directory) {
		free(node);
		return NULL;
	}

	fat32_directory_t * file = fat_directory_skip_name(directory);
	while (file->filename[0] == 0x2e || file->filename[0] == 0xe5 || file->filename[0] == 0xffffffe5) {
		directory = fat_next_entry(header, disk, directory, &cluster, buffer);
		file = fat_directory_skip_name(directory);
	}

	node->addr = fat_clust2addr(header, cluster) + (((char *) directory) - buffer);
	node->flags ^= node->flags & FS_FLAGS_ROOT;
	node->flags ^= node->flags & FS_FLAGS_DEVICE;
	node->flags |= (file->attributes & FAT_ATTRIBUTES_DEVICE) ? FS_FLAGS_DEVICE : 0;
	node->offset = 0;
	return node;
}

fs_node_t * fat_get_child(fs_t * fs, fs_node_t * node) {
	fs_node_t * child = malloc(sizeof(fs_node_t));
	if ((node->flags & FS_FLAGS_ROOT) == 0) {
		memcpy(child, node, sizeof(fs_node_t));
		return child;
	}

	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	uint32_t cluster = fat_addr2clust(header, node->addr);
	//dirent = fat_next_entry(header, disk, dirent, &cluster, buffer);
	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));

	while (directory) {
		fat32_directory_t * file = fat_directory_skip_name(directory);
		if (file->filename[0] == 0x2e || file->filename[0] == 0xe5 || file->filename[0] == 0xffffffe5) {
			directory = fat_next_entry(header, disk, directory, &cluster, buffer);
			continue;
		}
		node->addr = fat_clust2addr(header, cluster) + (((char *) directory) - buffer);
		node->flags ^= node->flags & FS_FLAGS_ROOT;
		node->flags ^= node->flags & FS_FLAGS_DEVICE;
		node->flags |= (file->attributes & FAT_ATTRIBUTES_DEVICE) ? FS_FLAGS_DEVICE : 0;
		node->offset = 0;
		return node;
	}
	return NULL;
}

fs_node_t * fat_backtrack(fs_t * fs, fs_node_t * node) {
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		return node;
	}
	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * dirent = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	drive_read_linear(disk, node->addr, buffer, 512);
	dirent = (fat32_directory_t *) buffer;

	fat32_directory_t * file = fat_directory_skip_name(dirent);
	if ((file->attributes & FAT_ATTRIBUTES_DIRECTORY) == 0) {
		return NULL;
	}

	uint32_t cluster = file->root_cluster_low | (file->root_cluster_high << 16);
	drive_read_linear(disk, fat_clust2addr(header, cluster), dirent, 512);
	fat_read_next(header, disk, cluster, buffer);

	uint32_t parent_cluster = 0;
	while (dirent) {
		fat32_directory_t * file = fat_directory_skip_name(dirent);
		if (memcmp(file->filename, "..         ", 11) == 0) {
			parent_cluster = file->root_cluster_low | (file->root_cluster_high << 16);
			break;
		}
		dirent = fat_next_entry(header, disk, dirent, &cluster, buffer);
	}
	if (parent_cluster == header->root_cluster || !dirent || parent_cluster == 0) {
		node->addr = fat_clust2addr(header, header->root_cluster);
		node->flags |= FS_FLAGS_ROOT;
		node->offset = 0;
		return node;
	}
	cluster = parent_cluster;
	drive_read_linear(disk, fat_clust2addr(header, cluster), buffer, 512);
	fat_read_next(header, disk, cluster, buffer);
	dirent = (fat32_directory_t *) buffer;

	uint32_t parent_parent_cluster = 0;
	while (dirent) {
		fat32_directory_t * file = fat_directory_skip_name(dirent);
		if (memcmp(file->filename, "..         ", 11) == 0) {
			parent_parent_cluster = file->root_cluster_low | (file->root_cluster_high << 16);
			break;
		}
		dirent = fat_next_entry(header, disk, dirent, &cluster, buffer);
	}
	if (!dirent || parent_parent_cluster == 0) {
		parent_parent_cluster = header->root_cluster;
	}

	cluster = parent_parent_cluster;
	drive_read_linear(disk, fat_clust2addr(header, cluster), buffer, 512);
	fat_read_next(header, disk, cluster, buffer);
	dirent = (fat32_directory_t *) buffer;
	while (dirent) {
		fat32_directory_t * file = fat_directory_skip_name(dirent);
		uint32_t entclust = file->root_cluster_low | (file->root_cluster_high << 16);
		if (entclust == parent_cluster) {
			node->addr = fat_clust2addr(header, cluster) + (((char *) dirent) - buffer);
			node->flags ^= node->flags & FS_FLAGS_ROOT;
			node->flags ^= node->flags & FS_FLAGS_DEVICE;
			node->flags |= (file->attributes & FAT_ATTRIBUTES_DEVICE) ? FS_FLAGS_DEVICE : 0;
			node->offset = 0;
			return node;
		}
		dirent = fat_next_entry(header, disk, dirent, &cluster, buffer);
	}

	return node;
}

void fat_name_chunkise(uint16_t * output, uint16_t * input) {
	int i = 0;
	memset(output, 0, 13);
	while (*input && i < 13) {
		*output++ = *input++;
		i++;
	}
}

fs_node_t * fat_create_dirent(fat32_directory_t * dirent, size_t size, uint16_t * name) {
	memset(dirent, 0, size);

	int extendeds = (size / 0x20) - 1;
	fat32_long_name_t * nameentry = (fat32_long_name_t *) dirent;
	uint16_t longname[13];
	for (int i = 0; i < extendeds; i++) {
		fat_name_chunkise(longname, name);
		memset(nameentry, 0, sizeof(fat32_long_name_t));
		nameentry->order = (i + 1) | ((i == (extendeds - 1)) ? 0x40 : 0);
		nameentry->attributes = 0x0f;
		nameentry->checksum = 0x94;
		if (longname[11] == 0) {
			longname[12] = 0xff;
		}
		memcpy(nameentry->first_chunk, longname, 10);
		memcpy(nameentry->second_chunk, longname + 5, 16);
		memcpy(nameentry->third_chunk, longname + 11, 4);
		name += 13;
		nameentry++;
	}

	char * shortname = "DOSFILE~TXT";
	dirent = (fat32_directory_t *) nameentry;
	memset(dirent, 0, sizeof(fat32_directory_t));
	memcpy(dirent->filename, shortname, 11);
}

uint8_t fat_flags2attr(uint32_t bits) {
	uint8_t attr = 0;
	attr |= (bits & STAT_FLAGS_RDONLY) ? FAT_ATTRIBUTES_RDONLY : 0;
	attr |= bits & STAT_FLAGS_DIRECTORY ? FAT_ATTRIBUTES_DIRECTORY : 0;
	attr |= (bits & STAT_FLAGS_DEVICE) ? FAT_ATTRIBUTES_DEVICE : 0;
	return attr;
}

void fat_make_dotfiles(char * buffer, uint32_t cluster, uint32_t parent_cluster) {
	fat32_directory_t * dot1 = (fat32_directory_t *) buffer;
	fat32_directory_t * dot2 = dot1 + 1;

	dot1->root_cluster_low = cluster & 0xffff;
	dot1->root_cluster_high = (cluster >> 16) & 0xffff;
	dot1->attributes = 0x10;
	memcpy(dot1->filename, ".          ", 11);

	dot2->root_cluster_low = parent_cluster & 0xffff;
	dot2->root_cluster_high = (parent_cluster >> 16) & 0xffff;
	dot2->attributes = 0x10;
	memcpy(dot2->filename, "..         ", 11);
}

fs_node_t * fat_create(fs_t * fs, fs_node_t * directory, fs_node_t * node, uint16_t * name, uint32_t flags) {
	drive_t * disk = fs->drive;
	int len = ustrlen(name) + 1;
	size_t required_size = 0x20 + 0x20 * (len / 13);
	if ((len % 13) != 0 ) {
		required_size += 0x20;
	}

	fat32_t * header = (fat32_t *) disk->sector1_cache;

	uint32_t cluster = fat_addr2clust(header, directory->addr);
	char buffer[1024];
	fat_read_node_clusters(header, directory, buffer);
	fat32_directory_t * dirent = (fat32_directory_t *) (buffer + fat_indexinto(header, directory->addr));
	if ((directory->flags & FS_FLAGS_ROOT) == 0) {
		drive_read_linear(disk, directory->addr, buffer, 512);
		dirent = (fat32_directory_t *) buffer;

		fat32_directory_t * file = fat_directory_skip_name(dirent);
		if ((file->attributes & FAT_ATTRIBUTES_DIRECTORY) == 0) {
			return NULL;
		}

		cluster = file->root_cluster_low | (file->root_cluster_high << 16);
		drive_read_linear(disk, fat_clust2addr(header, cluster), dirent, 512);
		fat_read_next(header, disk, cluster, buffer);
	}

	uint32_t parent_cluster = cluster == header->root_cluster ? 0 : cluster;
	while (dirent->filename[0] != 0) {
		dirent = fat_next_entry_unsafe(header, disk, dirent, &cluster, buffer);
	}
	fat_create_dirent(dirent, required_size, name);

	fat32_directory_t * file = fat_directory_skip_name(dirent);
	file->attributes = fat_flags2attr(flags);
	file->root_cluster_low = 0;
	file->root_cluster_high = 0;
	if (flags & STAT_FLAGS_DIRECTORY) {
		uint32_t contclust = fat_alloc_cluster(header, disk);
		file->root_cluster_low = contclust & 0xffff;
		file->root_cluster_high = (contclust >> 16) & 0xffff;

		char buffer2[512];
		memset(buffer2, 0x00, 512);
		fat_make_dotfiles(buffer2, contclust, parent_cluster);
		fat_write_cluster(header, disk, contclust, buffer2);
	}
	node->addr = fat_clust2addr(header, cluster) + (((char *) dirent) - buffer);
	node->flags ^= node->flags & FS_FLAGS_ROOT;
	node->flags ^= node->flags & FS_FLAGS_DEVICE;
	node->flags |= (file->attributes & FAT_ATTRIBUTES_DEVICE) ? FS_FLAGS_DEVICE: 0;
	node->offset = 0;
	fat_write_cluster(header, disk, cluster, buffer);
	fat_write_next(header, disk, cluster, buffer);
	return node;
}

void fat_set_flag(fs_t * fs, fs_node_t * node, uint32_t bits) {
	if ((node->flags & FS_FLAGS_ROOT) != 0) {
		return;
	}
	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;

	uint32_t cluster = fat_addr2clust(header, node->addr);
	char buffer[1024];
	fat_read_node_clusters(header, node, buffer);
	fat32_directory_t * directory = (fat32_directory_t *) (buffer + fat_indexinto(header, node->addr));
	directory = fat_directory_skip_name(directory);
	directory->attributes |= fat_flags2attr(bits);

	fat_write_cluster(header, disk, cluster, buffer);
	fat_write_next(header, disk, cluster, buffer);
}

fs_node_t * fat_get_root(fs_t * fs, fs_node_t * node) {
	drive_t * disk = fs->drive;
	fat32_t * header = (fat32_t *) disk->sector1_cache;
	node->addr = fat_clust2addr(header, header->root_cluster);
	node->flags |= FS_FLAGS_ROOT;
	node->offset = 0;
	return node;
}

int fat_detect_fs(drive_t * disk) {
	fat32_t * header = disk->sector1_cache;
	fat16_t * header16 = disk->sector1_cache;

	if ((header->signature != 0x28 && header->signature != 0x29) && (header16->signature != 0x28 && header16->signature != 0x29)) {
		return 0;
	}

	fat_size = 32; // fat_guess_size(header);
	return 1;
}

fs_t * fat_create_fs(drive_t * disk) {
	fs_t * fs = malloc(sizeof(fs_t));
	fs->search = fat_search_node;
	fs->write = fat_write;
	fs->read = fat_read;
	fs->stat = fat_stat;
	fs->get_root = fat_get_root;
	fs->read_name = fat_read_name;
	fs->iterate = fat_iterate;
	fs->get_child = fat_get_child;
	fs->backtrack = fat_backtrack;
	fs->create = fat_create;
	fs->set_flag = fat_set_flag;
	fs->drive = disk;
	return fs;
}

void fat_init() {
	fs_interface_t * interface = fs_create_interface();
	interface->detect = fat_detect_fs;
	interface->create = fat_create_fs;
	fs_register_interface(interface);
}