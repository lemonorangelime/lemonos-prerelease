#pragma once

#include <stdint.h>
#include <linked.h>

typedef struct drive drive_t;

typedef int (* drive_read_t)(drive_t * drive, uint64_t lba, void * buffer, size_t size);
typedef int (* drive_write_t)(drive_t * drive, uint64_t lba, void * buffer, size_t size);
typedef int (* drive_ioctl_t)(drive_t * drive, int request, int op, int op2, int op3, int op4);

typedef struct drive {
	int id;
	uint16_t * name;
	drive_read_t read;
	drive_write_t write;
	drive_ioctl_t ioctl;
	uint64_t lba_max;
	uint32_t sector_size;
	uint64_t sectors;
	uint64_t size;
	int present;
	int type;
	int locked;
	void * sector1_cache;
	void * priv;
} drive_t;

enum {
	DRIVE_FDD,
	DRIVE_HDD,
	DRIVE_SSD,
	DRIVE_CD,
	DRIVE_DVD,
	DRIVE_CF,
	DRIVE_RAM,
	DRIVE_SD,
};

enum {
	DRIVE_CMD_ENABLE,
	DRIVE_CMD_DISABLE,
	DRIVE_CMD_EJECT,
	DRIVE_CMD_INSERT,
	DRIVE_CMD_CACHE_ENABLE,
	DRIVE_CMD_CACHE_DISABLE,
	DRIVE_CMD_CACHE_FLUSH,
	DRIVE_CMD_CACHE_EVICT,
};

enum {
	DRIVE_LOCKED = -4,
};

extern linked_t * drives;

uint32_t lba2chs(uint32_t lba, int heads, int sectors);

drive_t * drive_alloc();
void drive_register(drive_t * drive);

int drive_read(drive_t * drive, uint64_t lba, void * buffer, size_t size);
int drive_write(drive_t * drive, uint64_t lba, void * buffer, size_t size);
int drive_read_linear(drive_t * drive, uint64_t address, void * dest, uint64_t size);
int drive_write_linear(drive_t * drive, uint64_t address, void * dest, uint64_t size);
int drive_ioctl(drive_t * drive, int request, int op, int op2, int op3, int op4);