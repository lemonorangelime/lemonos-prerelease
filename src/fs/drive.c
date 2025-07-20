#include <fs/drive.h>
#include <stdint.h>
#include <memory.h>
#include <linked.h>
#include <string.h>

linked_t * drives = NULL;

uint32_t lba2chs(uint32_t lba, int heads, int sectors) {
	uint32_t chs = 0;
	int modulo = (heads * sectors);
	chs |= (lba / modulo) << 16;
	chs |= ((lba / sectors) % heads) << 8;
	chs |= ((lba % sectors) + 1);
	return chs;
}

static int drive_request_id() {
	static int id = 0;
	return id++;
}

int drive_dummy_read(drive_t * drive, uint64_t lba, void * buffer, size_t size) {
	return -1;
}

int drive_dummy_write(drive_t * drive, uint64_t lba, void * buffer, size_t size) {
	return -1;
}

int drive_dummy_ioctl(drive_t * drive, int request, int op, int op2, int op3, int op4) {
	return -1;
}

drive_t * drive_alloc() {
	drive_t * drive = malloc(sizeof(drive_t));
	drive->id = drive_request_id();
	drive->lba_max = 0;
	drive->sector_size = 0;
	drive->sectors = 0;
	drive->present = 0;
	drive->locked = 0;
	drive->sector1_cache = NULL;
	drive->type = DRIVE_FDD;
	drive->read = drive_dummy_read;
	drive->write = drive_dummy_write;
	drive->ioctl = drive_dummy_ioctl;
	return drive;
}

int drive_read(drive_t * drive, uint64_t lba, void * buffer, size_t size) {
	if (drive->locked) {
		return DRIVE_LOCKED;
	}
	return drive->read(drive, lba, buffer, size);
}

int drive_write(drive_t * drive, uint64_t lba, void * buffer, size_t size) {
	if (drive->locked) {
		return DRIVE_LOCKED;
	}
	return drive->write(drive, lba, buffer, size);
}


int drive_read_linear(drive_t * drive, uint64_t address, void * dest, uint64_t size) {
	if (drive->locked) {
		return DRIVE_LOCKED;
	}
	int stat = 0;
	char buffer[512];
	uint64_t lba = (address >> 9) & 0b1111111111111111111111111111111111111111111111111111111;
	uint64_t offset = address & 0b111111111;
	uint64_t align = 512 - offset;
	stat = drive->read(drive, lba, buffer, 512);
	if (stat != 0) {
		return 1;
	}
	if (size < align) {
		memcpy(dest, buffer + offset, size);
		return stat;
	}
	memcpy(dest, buffer + offset, align);
	dest += align;
	size -= align;
	lba++;
	while ((stat == 0) && size >= 512) {
		stat = drive->read(drive, lba++, buffer, 512);
		memcpy(dest, buffer, 512);
		dest += 512;
		size -= 512;
	}
	if (size == 0) {
		return stat;
	}
	stat = drive->read(drive, lba, buffer, 512);
	memcpy(dest, buffer, size);
	return stat;
}



int drive_write_linear(drive_t * drive, uint64_t address, void * dest, uint64_t size) {
	int stat = 0;
	char buffer[512];
	uint64_t lba = (address >> 9) & 0b1111111111111111111111111111111111111111111111111111111;
	uint64_t offset = address & 0b111111111;
	uint64_t align = 512 - offset;
	stat = drive->read(drive, lba, buffer, 512);
	if (stat != 0) {
			return 1;
	}
	if (size < align) {
			memcpy(buffer + offset, dest, size);
			stat = drive->write(drive, lba, buffer, 512);
			return stat;
	}
	memcpy(buffer + offset, dest, align);
	stat = drive->write(drive, lba, buffer, 512);
	dest += align;
	size -= align;
	lba++;
	while ((stat == 0) && size >= 512) {
			stat = drive->write(drive, lba++, buffer, 512);
			dest += 512;
			size -= 512;
	}
	if (size == 0) {
			return stat;
	}
	stat = drive->read(drive, lba, buffer, 512);
	if (stat != 0) {
			return 1;
	}
	memcpy(buffer, dest, size);
	stat = drive->write(drive, lba, buffer, 512);
	return stat;
}



int drive_ioctl(drive_t * drive, int request, int op, int op2, int op3, int op4) {
	if (drive->locked) {
		return DRIVE_LOCKED;
	}
	return drive->ioctl(drive, request, op, op2, op3, op4);
}

void drive_register(drive_t * drive) {
	drives = linked_add(drives, drive);
}