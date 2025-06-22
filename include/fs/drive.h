#pragma once

#include <stdint.h>

typedef struct drive drive_t;
typedef int (* drive_read_t)(drive_t * drive, uint64_t lba, void * buffer, size_t size);
typedef int (* drive_write_t)(drive_t * drive, uint64_t lba, void * buffer, size_t size);

typedef struct drive {
	int id;
	drive_read_t read; // read lba
	drive_write_t write; // write lba
	uint32_t lba_max; // max lba address
	uint32_t sector_size; // bytes per sector
	uint32_t sectors; // sector count
	uint64_t size; // drive byte count
	int present;
	int type; // FLOPPY ή ATA
	int format; // disk format (FAT12, LFS)
	void * priv; // private
} drive_t;

enum {
	DRIVE_FLOPPY,
	DRIVE_HARDDISK,
	DRIVE_SSD,
};

enum {
	DRIVE_CMD_ENABLE,
	DRIVE_CMD_DISABLE,
	DRIVE_CMD_EJECT,
	DRIVE_CMD_INSERT,
	DRIVE_CMD_HINT_EJECT,
	DRIVE_CMD_HINT_INSERT,
	DRIVE_CMD_CACHE_ENABLE,
	DRIVE_CMD_CACHE_DISABLE,
	DRIVE_CMD_CACHE_FLUSH,
	DRIVE_CMD_CACHE_EVICT,
};

drive_t * drive_make_interface();
int drive_read(int id, void * buffer, size_t size);
int drive_write(int id, void * buffer, size_t size);
void drive_ctrl(int id);
void drive_register_interface(drive_t * drive);
uint32_t lba2chs(uint32_t lba, int heads, int sectors);