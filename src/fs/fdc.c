#include <fs/fdc.h>
#include <asm.h>
#include <interrupts/irq.h>
#include <ports.h>
#include <cmos.h>
#include <dma.h>
#include <pit.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <fs/drive.h>
#include <cpuspeed.h>
#include <graphics/graphics.h>

// this shit is absolutely fucked

static volatile int irq_recieved = 0;
static int fdc_port = FDC_PORT;
static int drives[2];
static int selected;

void fdc_irq(registers_t * regs) {
	irq_recieved = 1;
}

void fdc_outb(int port, uint8_t data) {
	outb(fdc_port + port, data);
}

uint8_t fdc_inb(int port) {
	return inb(fdc_port + port);
}

void fdc_wait() {
	while (irq_recieved == 0) {}
	irq_recieved = 0;
}

void fdc_dma(int direction, uint16_t count) {
	uint32_t address = FDC_DMA;
	count -= 1;
	outb(0x0a, 0x06);
	outb(0x0c, 0xff);
	outb(0x04, address & 0xff);
	outb(0x04, (address >> 8) & 0xff);
	outb(0x81, (address >> 16) & 0xff);
	outb(0x0c, 0xff);
	outb(0x05, count & 0xff);
	outb(0x05, (count >> 8) & 0xff);
	outb(0x0b, direction == FDC_CMD_READ ? 0x46 : 0x4a);
	outb(0x0a, 0x02);
}

void fdc_select(int disk) {
	if (selected == disk) {
		return;
	}
	fdc_outb(FDC_PORT_OUT, 0x0c | disk);
	fdc_outb(FDC_PORT_CONF, fdc_datarate(drives[disk]));
	selected = disk;
}

int fdc_arbitrary(int disk, int cylinder, int head, int sector, int direction, void * buffer, size_t count) {
	int ret = 1;
	fdc_select(disk);
	if (fdc_seek(disk, cylinder, 0)) {
		return 1;
	}
	if (fdc_seek(disk, cylinder, 1)) {
		return 1;
	}
	int type = drives[disk];
	int sectors = fdc_tracksectors(type);
	if (direction == FDC_CMD_WRITE) {
		memcpy((void *) FDC_DMA, buffer, count);
	}
	for (int i = 0; i < 20; i++) {
		fdc_motor(disk, 1);
		fdc_dma(direction, count);
		fdc_cmd(direction); // FDC_CMD_WRITE ή FDC_CMD_READ
		fdc_cmd((head << 2) | disk);
		fdc_cmd(cylinder);
		fdc_cmd(head);
		fdc_cmd(sector);
		fdc_cmd(2);
		fdc_cmd(sectors);
		fdc_cmd(0x1b);
		fdc_cmd(0xff);
		fdc_wait();
		uint8_t stat0 = fdc_read();
		uint8_t stat1 = fdc_read();
		uint8_t stat2 = fdc_read();

		fdc_read();
		fdc_read();
		fdc_read();
		uint8_t sectlen = fdc_read();

		if (sectlen == 0x02 && !(stat0 & 0xc8) && !(stat1 & 0b10110111) && !(stat2 & 0b01110111)) {
			ret = 0;
			break;
		}
	}
	fdc_motor(disk, 0);
	if (direction == FDC_CMD_READ && ret == 0) {
		memcpy(buffer, (void *) FDC_DMA, count);
	}
	return ret;
}

int fdc_sector(int disk, int cylinder, int head, int sector, int direction, void * buffer) {
	int ret = 1;
	fdc_select(disk);
	if (fdc_seek(disk, cylinder, 0)) {
		return 1;
	}
	if (fdc_seek(disk, cylinder, 1)) {
		return 1;
	}
	int type = drives[disk];
	int sectors = fdc_tracksectors(type);
	if (direction == FDC_CMD_WRITE) {
		memcpy((void *) FDC_DMA, buffer, FDC_SECTOR_SIZE);
	}
	for (int i = 0; i < 20; i++) {
		fdc_motor(disk, 1);
		fdc_dma(direction, FDC_SECTOR_SIZE);
		fdc_cmd(direction); // FDC_CMD_WRITE ή FDC_CMD_READ
		fdc_cmd((head << 2) | disk);
		fdc_cmd(cylinder);
		fdc_cmd(head);
		fdc_cmd(sector);
		fdc_cmd(2);
		fdc_cmd(sectors);
		fdc_cmd(0x1b);
		fdc_cmd(0xff);
		fdc_wait();
		uint8_t stat0 = fdc_read();
		uint8_t stat1 = fdc_read();
		uint8_t stat2 = fdc_read();

		fdc_read();
		fdc_read();
		fdc_read();
		uint8_t sectlen = fdc_read();

		if (sectlen == 0x02 && !(stat0 & 0xc8) && !(stat1 & 0b10110111) && !(stat2 & 0b01110111)) {
			ret = 0;
			break;
		}
	}
	fdc_motor(disk, 0);
	if (direction == FDC_CMD_READ && ret == 0) {
		memcpy(buffer, (void *) FDC_DMA, FDC_SECTOR_SIZE);
	}
	return ret;
}

int fdc_track(int disk, int cylinder, int head, int direction, void * buffer) {
	int ret = 1;
	fdc_select(disk);
	if (fdc_seek(disk, cylinder, 0)) {
		return 1;
	}
	if (fdc_seek(disk, cylinder, 1)) {
		return 1;
	}
	int type = drives[disk];
	int sectors = fdc_tracksectors(type);
	int tracklen = sectors * FDC_SECTOR_SIZE;
	if (direction == FDC_CMD_WRITE) {
		memcpy((void *) FDC_DMA, buffer, tracklen);
	}
	for (int i = 0; i < 20; i++) {
		fdc_motor(disk, 1);
		fdc_dma(direction, tracklen);
		fdc_cmd(direction); // FDC_CMD_WRITE ή FDC_CMD_READ
		fdc_cmd((head << 2) | disk);
		fdc_cmd(cylinder);
		fdc_cmd(head);
		fdc_cmd(1);
		fdc_cmd(2);
		fdc_cmd(sectors);
		fdc_cmd(0x1b);
		fdc_cmd(0xff);
		fdc_wait();
		uint8_t stat0 = fdc_read();
		uint8_t stat1 = fdc_read();
		uint8_t stat2 = fdc_read();

		fdc_read();
		fdc_read();
		fdc_read();
		uint8_t sectlen = fdc_read();

		if (sectlen == 0x02 && !(stat0 & 0xc8) && !(stat1 & 0b10110111) && !(stat2 & 0b01110111)) {
			ret = 0;
			break;
		}
	}
	fdc_motor(disk, 0);
	if (direction == FDC_CMD_READ && ret == 0) {
		memcpy(buffer, (void *) FDC_DMA, tracklen);
	}
	return ret;
}

int fdc_disk_read(int disk, int cylinder, int head, int sector, void * buffer, size_t size) {
	int type = drives[disk];
	int sectors = fdc_tracksectors(type);
	int tracklen = sectors * FDC_SECTOR_SIZE;
	if (size < tracklen) { // we can read a whole track or sector easily
		return fdc_arbitrary(disk, cylinder, head, sector, FDC_CMD_READ, buffer, size);
	}
	int tracks = size / tracklen;
	int remainder = size % tracklen;
	int ret = 0;
	for (int i = 0; i < tracks; i++) {
		int size = tracklen - (sector * 512);
		ret += fdc_arbitrary(disk, cylinder, head, sector, FDC_CMD_READ, buffer, size);
		buffer += size;
		sector = 0;
		cylinder += 1;
		if (cylinder > 39) {
			head = 1;
			cylinder = 0;
		}
	}
	return ret;
}

int fdc_disk_write(int disk, int cylinder, int head, int sector, void * buffer, size_t size) {
	int type = drives[disk];
	int sectors = fdc_tracksectors(type);
	int tracklen = sectors * FDC_SECTOR_SIZE;
	if (size < tracklen) { // if its smaller than a track we can just do this, shrimple!
		return fdc_arbitrary(disk, cylinder, head, sector, FDC_CMD_WRITE, buffer, size);
	}
	int tracks = size / tracklen;
	int remainder = size % tracklen;
	int ret = 0;
	for (int i = 0; i < 2880; i++) {
		uint32_t chs = lba2chs(i, 2, 18);
		uint8_t sector = chs & 0xff;
		uint8_t head = (chs >> 8) & 0xff;
		uint16_t cylinder = (chs >> 16) & 0xffff;
		ret += fdc_sector(disk, cylinder, head, sector, FDC_CMD_WRITE, buffer);
		buffer += FDC_SECTOR_SIZE; // stupid
	}
	return ret;
	for (int i = 0; i < tracks; i++) {
		int chunk = tracklen - (sector * FDC_SECTOR_SIZE); // size (track aligned)
		ret += fdc_arbitrary(disk, cylinder, head, sector, FDC_CMD_WRITE, buffer, chunk);
		cpuspeed_wait_tsc(cpu_hz / 100);
		buffer += chunk; // stupid
		sector = 1; // we dont care about starting sector now (track aligned)
		head += 1; // track on other side of disk
		if (head >= 2) {
			cylinder += 1; // next cylinder (both sides written now)
		}
	}
	return ret;
}

// lol
void fdc_perpendiculate() {
	uint8_t disks = 0;
	if (drives[0] == FDC_2_88MB) {
		disks |= 0b00001;
	}
	if (drives[1] == FDC_2_88MB) {
		disks |= 0b00010;
	}
	fdc_cmd(FDC_CMD_PERPENDICULAR);
	fdc_cmd(disks << 2);
}

int fdc_reset(int disks) {
	int ret = 0;
	fdc_outb(FDC_PORT_OUT, 0x00);
	fdc_outb(FDC_PORT_OUT, 0x0c);
	fdc_wait();
	fdc_sense();
	fdc_outb(FDC_PORT_CONF, 0x00);
	fdc_cmd(FDC_CMD_SELECT);
	fdc_cmd(0xdf);
	fdc_cmd(0x02);
	for (int i = 0; i <= disks; i++) {
		int calibrate_status = 0;
		fdc_select(i);
		calibrate_status = fdc_calibrate(i);
		if (calibrate_status) {
			drives[i] = 0;
		}
		ret += calibrate_status;
	}
	fdc_perpendiculate();
	return ret;
}

int fdc_calibrate(int disk) {
	int ret = 1;
	fdc_select(disk);
	fdc_motor(disk, 1);
	for (int i = 0; i < 10; i++) {
		fdc_cmd(FDC_CMD_CALIBRATE);
		fdc_cmd(disk);
		fdc_wait();

		uint16_t sense = fdc_sense();
		if (sense & 0xc0) {
			continue;
		}
		if ((sense >> 8) == 0) {
			ret = 0;
			break;
		}
	}
	fdc_motor(disk, 0);
	return ret;
}

int fdc_seek(int disk, int cylinder, int head) {
	int ret = 1;
	fdc_select(disk);
	fdc_motor(disk, 1);
	for (int i = 0; i < 10; i++) {
		fdc_cmd(FDC_CMD_SEEK);
		fdc_cmd((head << 2) | disk);
		fdc_cmd(cylinder);
		fdc_wait();

		uint16_t sense = fdc_sense();
		if (sense & 0xc0) {
			continue;
		}
		if ((sense >> 8) == cylinder) {
			ret = 0;
			break;
		}
	}
	fdc_motor(disk, 0);
	return ret;
}

int fdc_detect(int disk) {
	int ret = 1;
	fdc_select(disk);
	for (int i = 0; i < 1; i++) {
		fdc_cmd(FDC_CMD_DETECT);
		// fdc_cmd(disk);
		// fdc_wait();

		uint16_t sense = fdc_sense();
		// LemonOS has a stupid serial print here for debugging
	}
	return ret;
}

void fdc_motor(int disk, int power) {
	fdc_select(disk);
	fdc_outb(FDC_PORT_OUT, (power << (4 + disk)) | FDC_OUT_IRQ | FDC_OUT_RESET);
}

uint16_t fdc_sense() {
	uint16_t ret;
	fdc_cmd(FDC_CMD_SENSE);
	ret |= fdc_read(); // status
	ret |= fdc_read() << 8; // cylinder
	return ret;
}

int fdc_cmd(int cmd) {
	uint64_t target = pit_freq * 60;
	while (target > ticks) {
		if (fdc_inb(FDC_PORT_STATUS) & FDC_STATUS_READY) {
			fdc_outb(FDC_PORT_FIFO, cmd);
			return 0;
		}
	}
	return 1;
}

uint8_t fdc_read() {
	uint64_t target = pit_freq * 60;
	while (target > ticks) {
		if (fdc_inb(FDC_PORT_STATUS) & FDC_STATUS_READY) {
			return fdc_inb(FDC_PORT_FIFO);
		}
	}
	return 0;
}

int fdc_datarate(int type) {
	switch (type) {
		case FDC_NONE:
			return -1; // obviously no data without a drive you fuckng moron
		case FDC_2_88MB:
			return FDC_1MBPS | 0x40;
		case FDC_1_44MB:
		case FDC_1_2MB:
			return FDC_500KBPS;
		case FDC_720KB:
		case FDC_360KB:
			return FDC_250KBPS;
	}
}

int fdc_tracksectors(int type) {
	switch (type) {
		case FDC_NONE:
			return 0; // obviously no sectors without a drive you fuckng moron
		case FDC_2_88MB:
			return 36;
		case FDC_1_44MB:
			return 18;
		case FDC_1_2MB:
			return 15;
		case FDC_720KB:
			return 9;
		case FDC_360KB:
			return 9;
	}
}

void fdc_init() {
	uint8_t floppy = cmos_read_register(CMOS_FLOPPY);
	if (floppy == 0) {
		return;
	}
	irq_set_handler(FDC_IRQ, fdc_irq);
	drives[0] = floppy >> 4;
	drives[1] = floppy & 0x0f;
	fdc_reset(1);
	fdc_calibrate(0);

	char sector[512];
	fdc_sector(0, 0, 0, 1, FDC_CMD_READ, sector);
	printf(u"Sector 1: %8\n", sector);
}
