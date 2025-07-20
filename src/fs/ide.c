#include <fs/ide.h>
#include <bus/pci.h>
#include <string.h>
#include <memory.h>
#include <ports.h>
#include <stdio.h>
#include <util.h>
#include <fs/drive.h>
#include <interrupts/irq.h>
#include <cpuspeed.h>
#include <string.h>

static int ide_irq_triggered = 0;

ide_disk_t * init_disk(ide_t * ide, int channel, int drive) {
	ide_disk_t * disk = malloc(sizeof(ide_disk_t));
	disk->controller = ide;
	disk->command_set = 0;
	disk->channel = channel;
	disk->drive = drive;
	disk->caps = 0;
	disk->present = 0;
	disk->signature = 0;
	disk->size = 0;
	disk->lba_mode = 0;
	disk->type = IDE_ATA;
	disk->mode = IDE_MODE_CHS;
	return disk;
}

uint8_t ide_io_inb(ide_disk_t * disk, uint8_t reg) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	return inb(base + reg);
}

uint16_t ide_io_inw(ide_disk_t * disk, uint8_t reg) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	return inw(base + reg);
}

uint8_t ide_io_outb(ide_disk_t * disk, uint8_t reg, uint8_t data) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	outb(base + reg, data);
}

uint8_t ide_io_outw(ide_disk_t * disk, uint8_t reg, uint16_t data) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	outw(base + reg, data);
}

uint16_t ide_ctrl_inb(ide_disk_t * disk, uint8_t reg) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar3 : controller->bar1;
	return inb(base + reg);
}

uint16_t ide_ctrl_inw(ide_disk_t * disk, uint8_t reg) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar3 : controller->bar1;
	return inw(base + reg);
}

uint8_t ide_ctrl_outb(ide_disk_t * disk, uint8_t reg, uint8_t data) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar3 : controller->bar1;
	outb(base + reg, data);
}

void ide_io_inb_buffer(ide_disk_t * disk, uint8_t reg, void * buffer, uint32_t size) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	uint32_t * p = buffer;
	size >>= 2;
	while (size--) {
		*p++ = ind(base + reg);
	}
}

void ide_io_inw_buffer(ide_disk_t * disk, uint8_t reg, void * buffer, uint32_t size) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	uint16_t * p = buffer;
	size >>= 1;
	while (size--) {
		*p++ = inw(base + reg);
	}
}

void ide_io_outw_buffer(ide_disk_t * disk, uint8_t reg, void * buffer, uint32_t size) {
	ide_t * controller = disk->controller;
	uint16_t base = disk->channel ? controller->bar2 : controller->bar0;
	uint16_t * p = buffer;
	size >>= 1;
	while (size--) {
		outw(base + reg, *p++);
	}
}

void ide_select(ide_disk_t * disk) {
	ide_io_outb(disk, IDE_IO_REG_SELECT, disk->drive << 4);
}


void ide_callback(registers_t * regs) {
	ide_irq_triggered = 1;
}

void ide_poll_irq() {
	while (!ide_irq_triggered) {}
	ide_irq_triggered = 0;
}

void ide_try_native(pci_t * pci) {
	uint8_t interface = pci->interface;
	uint8_t mutable = interface & 0b0010;
	if (!mutable) {
		return;
	}
	//printf(u"IDE: switching to native mode\n");
	pci_config_outb(pci->bus, pci->slot, pci->function, 0x09, interface | 1);
	pci_resync(pci);
}

uint8_t ide_direction2cmd(int mode, int direction) {
	switch (mode) {
		case IDE_MODE_CHS:
			return (direction == IDE_READ) ? IDE_CMD_READ_PIO : IDE_CMD_WRITE_PIO;
		case IDE_MODE_LBA:
			return (direction == IDE_READ) ? IDE_CMD_READ_PIO : IDE_CMD_WRITE_PIO;
		case IDE_MODE_LBA48:
			return (direction == IDE_READ) ? IDE_CMD_READ_PIO_EXT : IDE_CMD_WRITE_PIO_EXT;
	}
}

int ide_poll(ide_disk_t * disk, int direction) {
	//cpuspeed_wait_tsc(cpu_hz / 2500000);

	while (ide_io_inb(disk, IDE_IO_REG_STATUS) & IDE_STATUS_BUSY) {}

	if (direction == IDE_READ) {
		uint8_t status = ide_io_inb(disk, IDE_IO_REG_STATUS);
		if ((status & IDE_STATUS_ERROR) || (status & IDE_STATUS_FAULT)) {
			return 1;
		}
		if ((status & IDE_STATUS_REQUEST) == 0) {
			return 1;
		}
	}
	return 0;
}

int ide_disk_read(drive_t * drive, uint64_t lba, void * buffer, size_t size) {
	ide_disk_t * disk = drive->priv;
	switch (disk->type) {
		case IDE_ATA: return ide_ata_do_sector(disk, IDE_READ, lba, size >> 9, buffer);
		case IDE_ATAPI: return ide_atapi_do_sector(disk, IDE_READ, lba, size >> 11, buffer, 0);
	}
}

int ide_disk_write(drive_t * drive, uint64_t lba, void * buffer, size_t size) {
	ide_disk_t * disk = drive->priv;
	switch (disk->type) {
		case IDE_ATA: return ide_ata_do_sector(disk, IDE_WRITE, lba, size >> 9, buffer);
		case IDE_ATAPI: return ide_atapi_do_sector(disk, IDE_WRITE, lba, size >> 11, buffer, 0);
	}
}

int ide_disk_ioctl(drive_t * drive, int request, int op, int op2, int op3, int op4) {
	return -1;
}

int ide_atapi_detect(ide_disk_t * disk) {
	return 1;
}

drive_t * ide_detect(ide_disk_t * disk) {
	ide_select(disk);
	ide_io_outb(disk, IDE_IO_REG_COMMAND, IDE_CMD_IDENTIFY);
	sleep_seconds(1);

	uint8_t status = ide_io_inb(disk, IDE_IO_REG_STATUS);
	if (status == 0) {
		return NULL;
	}
	status = ide_io_inb(disk, IDE_IO_REG_STATUS);

	int working = status & IDE_STATUS_ERROR;
	if (working) {
		uint8_t lba1 = ide_io_inb(disk, IDE_IO_REG_LBA1);
		uint8_t lba2 = ide_io_inb(disk, IDE_IO_REG_LBA2);
		if ((lba1 == 0x14 && lba2 == 0xeb) || (lba1 == 0x69 && lba2 == 0x96)) {
			disk->type = IDE_ATAPI;
		} else {
			return NULL;
		}
		ide_io_outb(disk, IDE_IO_REG_COMMAND, IDE_CMD_IDENTIFY_PACKET);
		sleep_seconds(1);
	}
	char buffer[512];
	ide_ident_t * identity = (ide_ident_t *) &buffer;
	ide_io_inb_buffer(disk, IDE_IO_REG_DATA, identity, 512);
	sleep_seconds(1);

	disk->present = 1;
	disk->caps = identity->caps;
	disk->command_set = identity->cmd_sets;
	if ((identity->cmd_sets >> 26) & 1) {
		disk->size = identity->max_lba48;
		disk->mode = IDE_MODE_LBA48;
	} else {
		disk->size = identity->max_lba;
		disk->mode = (disk->caps & 0x200) ? IDE_MODE_LBA : IDE_MODE_CHS;
	}

	uint16_t * name = malloc(42 * 2);
	for (int i = 0; i < 40; i += 2) {
		name[i + 1] = identity->model[i];
		name[i] = identity->model[i + 1];
	}
	name[40] = 0;

	printf(u"IDE: detected %s device: %s\n", disk->type == IDE_ATA ? u"ATA" : u"ATAPI", name);

	drive_t * drive = drive_alloc();
	drive->name = name;
	drive->lba_max = disk->size;
	drive->present = (disk->type == IDE_ATA) ? 1 : ide_atapi_detect(disk);
	drive->priv = disk;
	drive->sector_size = (disk->type == IDE_ATA) ? 512 : 2048;
	drive->sectors = drive->lba_max;
	drive->size = drive->lba_max * drive->sector_size;
	drive->type = (disk->type == IDE_ATA) ? DRIVE_HDD : DRIVE_CD; // todo: detect what the media actually is (doesnt actually matter so we just assume CD for now)
	drive->read = ide_disk_read;
	drive->write = ide_disk_write;
	drive->ioctl = ide_disk_ioctl;
	drive->sector1_cache = malloc(drive->sector_size);
	return drive;
}

int ide_ata_do_sector(ide_disk_t * disk, int direction, uint64_t lba, uint8_t sectors, void * buffer) {
	uint8_t select_byte = (disk->drive << 4);
	uint64_t lba_bits = lba;
	int cmd = ide_direction2cmd(disk->mode, direction);
	switch (disk->mode) {
		case IDE_MODE_CHS:
			select_byte |= 0xa0;
			break;
		case IDE_MODE_LBA:
			select_byte |= 0xe0 | (lba >> 24) & 0xF;
			break;
		case IDE_MODE_LBA48:
			uint8_t sector = (lba % 63) + 1;
			uint16_t cylinder = (lba + 1  - sector) / (16 * 63);
			select_byte |= 0xe0;
			lba_bits = (cylinder << 16) | (cylinder << 8) | sector;
			break;
	}

	while (ide_io_inb(disk, IDE_IO_REG_STATUS) & IDE_STATUS_BUSY) {}
	ide_io_outb(disk, IDE_IO_REG_SELECT, select_byte);

	if (disk->mode == IDE_MODE_LBA48) {
		ide_io_outb(disk, IDE_IO_REG_SECTOR_COUNT, 0);
		ide_io_outb(disk, IDE_IO_REG_LBA0, (lba_bits >> 24) & 0xff);
		ide_io_outb(disk, IDE_IO_REG_LBA1, (lba_bits >> 32) & 0xff);
		ide_io_outb(disk, IDE_IO_REG_LBA2, (lba_bits >> 40) & 0xff);
	}
	ide_io_outb(disk, IDE_IO_REG_SECTOR_COUNT, sectors);
	ide_io_outb(disk, IDE_IO_REG_LBA0, lba_bits & 0xff);
	ide_io_outb(disk, IDE_IO_REG_LBA1, (lba_bits >> 8) & 0xff);
	ide_io_outb(disk, IDE_IO_REG_LBA2, (lba_bits >> 16) & 0xff);
	ide_io_outb(disk, IDE_IO_REG_COMMAND, cmd);

	ide_transfer_function_t transfer = (direction == IDE_READ) ? ide_io_inw_buffer : ide_io_outw_buffer;
	while (sectors--) {
		if (ide_poll(disk, direction)) {
			return 1;
		}
		transfer(disk, IDE_IO_REG_DATA, buffer, 512);
		buffer += 512;
	}
	//ide_io_outb(disk, IDE_IO_REG_COMMAND, disk->mode == IDE_MODE_LBA48 ? IDE_CMD_FLU : );
	if (direction == IDE_WRITE) {
		ide_poll(disk, IDE_WRITE);
	}
	return 0;
}

int ide_atapi_do_sector(ide_disk_t * disk, int direction, uint64_t lba, uint8_t sectors, void * buffer, uint16_t feature) {
	uint16_t length = sectors * 2048;

	ide_io_outb(disk, IDE_IO_REG_SELECT, disk->drive << 4);
	cpuspeed_wait_tsc(cpu_hz / 2500000);

	ide_io_outb(disk, IDE_IO_REG_FEATURES, 0);
	ide_io_outb(disk, IDE_IO_REG_LBA1, length & 0xff);
	ide_io_outb(disk, IDE_IO_REG_LBA2, (length >> 8) & 0xff);
	ide_io_outb(disk, IDE_IO_REG_COMMAND, IDE_CMD_PACKET);
	cpuspeed_wait_tsc(cpu_hz / 2500000);

	if (ide_poll(disk, IDE_READ)) {
		return 1;
	}

	ide_io_outw(disk, IDE_IO_REG_DATA, ((direction == IDE_READ) ? IDE_CMD_READ_ATAPI : IDE_CMD_WRITE_ATAPI));
	ide_io_outw(disk, IDE_IO_REG_DATA, htonw((lba >> 16) & 0xffff));
	ide_io_outw(disk, IDE_IO_REG_DATA, htonw(lba & 0xffff));
	ide_io_outw(disk, IDE_IO_REG_DATA, 0);
	ide_io_outw(disk, IDE_IO_REG_DATA, htonw(sectors));
	ide_io_outw(disk, IDE_IO_REG_DATA, feature);

	ide_transfer_function_t transfer = (direction == IDE_READ) ? ide_io_inw_buffer : ide_io_outw_buffer;
	while (sectors--) {
		if (ide_poll(disk, IDE_READ)) {
			return 1;
		}
		transfer(disk, IDE_IO_REG_DATA, buffer, 2048);
		buffer += 2048;
	}
	return 0;
}

void ide_reset(ide_disk_t * disk) {
	drive_t * drive = ide_detect(disk);
	if (!drive) {
		return;
	}
	drive_read(drive, 0, drive->sector1_cache, drive->sector_size);
	drive_register(drive);
}

int ide_dev_init(pci_t * pci) {
	uint8_t interface = pci->interface;
	int native = (interface & 0b0001);
	//printf(u"IDE: found IDE controller (%s)\n", native ? u"native" : u"compatibility");
	if (!native) {
		//printf(u"IDE: %s switch to native mode\n", (interface & 0b0010) ? u"CAN" : u"can NOT");
	}
	ide_try_native(pci);
	//printf(u"IDE: DMA is %s\n", (interface & 0b10000000) ? u"supported" : u"unsupported");

	ide_t * device = malloc(sizeof(ide_t));
	device->native = native;
	device->dma = 0;
	device->bar0 = native ? pci->bar0 : 0x01f0;
	device->bar1 = native ? pci->bar1 : 0x03f6;
	device->bar2 = native ? pci->bar2 : 0x0170;
	device->bar3 = native ? pci->bar3 : 0x0376;
	device->bar4 = native ? pci->bar4 : 0;

	device->disks[0] = init_disk(device, 0, 0);
	device->disks[1] = init_disk(device, 0, 1);
	device->disks[2] = init_disk(device, 1, 0);
	device->disks[3] = init_disk(device, 1, 1);

	ide_reset(device->disks[0]);
	ide_reset(device->disks[1]);
	ide_reset(device->disks[2]);
	ide_reset(device->disks[3]);
}

int ide_check(pci_t * pci) {
	if (pci->class == 0x01 && pci->subclass == 0x01) {
		return 1;
	}
	return 0;
}

void ide_init() {
	irq_set_handler(46, ide_callback);
	irq_set_handler(47, ide_callback);
	pci_add_handler(ide_dev_init, ide_check);
}
