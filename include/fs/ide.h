#pragma once

#include <stdint.h>
#include <bus/pci.h>

typedef struct ide ide_t;
typedef struct ide_disk ide_disk_t;

typedef void (* ide_transfer_function_t)(ide_disk_t * disk, uint8_t reg, void * buffer, uint32_t size);

typedef struct ide_disk {
	ide_t * controller;
	int present;
	int channel;
	int drive;
	int type;
	int mode;
	int lba_mode;
	uint16_t signature;
	uint16_t caps;
	uint32_t command_set;
	uint64_t size;
} ide_disk_t;

typedef struct {
	uint16_t type;
	uint16_t cylinders;
	uint16_t reserved;
	uint16_t heads;
	uint16_t unformatted_track_length;
	uint16_t unformatted_sector_length;
	uint16_t sectors_per_track;
	uint16_t sector_gap;
	uint16_t phase_lock_oscillator_size;
	uint16_t vendor_words;
	char serial[20];
	uint16_t controller_type;
	uint16_t buffer_size;
	uint16_t ecc_byte_length;
	char firmware[8];
	char model[40];
	uint16_t multiple_ops;
	uint16_t dword_supported;
	uint16_t caps;
	uint16_t reserved1;
	uint16_t min_pio_cycle_time;
	uint16_t min_dma_cycle_time;

	uint16_t extended_ident;
	uint16_t cylinder_count;
	uint16_t head_count;
	uint16_t sectors_per_track2;
	uint32_t max_lba;
	uint32_t max_max_lba;
	uint16_t reserved2[42];
	uint32_t cmd_sets;
	uint16_t reserved3[2];
	uint32_t cmd_sets_enabled;
	uint16_t reserved4[14];
	uint64_t max_lba48;
} __attribute__((packed)) ide_ident_t;

typedef struct ide {
	int native;
	int dma;
	pci_t * pci;
	uint32_t bar0;
	uint32_t bar1;
	uint32_t bar2;
	uint32_t bar3;
	uint32_t bar4;
	ide_disk_t * disks[4];
} ide_t;

enum {
	IDE_STATUS_ERROR		= 0b00000001,
	IDE_STATUS_INDEX		= 0b00000010,
	IDE_STATUS_CORRECTED	= 0b00000100,
	IDE_STATUS_REQUEST		= 0b00001000,
	IDE_STATUS_SEEK			= 0b00010000,
	IDE_STATUS_FAULT		= 0b00100000,
	IDE_STATUS_READY		= 0b01000000,
	IDE_STATUS_BUSY			= 0b10000000,
};

enum {
	IDE_ERROR_NOADDRMARK	= 0b00000001,
	IDE_ERROR_TRACK0		= 0b00000010,
	IDE_ERROR_ABORTED		= 0b00000100,
	IDE_ERROR_EJECT_REQUEST	= 0b00001000,
	IDE_ERROR_NOIDMARK		= 0b00010000,
	IDE_ERROR_EJECTED		= 0b00100000,
	IDE_ERROR_CRC			= 0b01000000,
	IDE_ERROR_BAD_BLOCK		= 0b10000000,
};
/*
enum {
	IDE_ERROR_NOADDRMARK	= 0b00000001,
	IDE_ERROR_TRACK0		= 0b00000010,
	IDE_ERROR_ABORTED		= 0b00000100,
	IDE_ERROR_EJECT_REQUEST	= 0b00001000,
	IDE_ERROR_NOIDMARK		= 0b00010000,
	IDE_ERROR_EJECTED		= 0b00100000,
	IDE_ERROR_CRC			= 0b01000000,
	IDE_ERROR_BAD_BLOCK		= 0b10000000,
};*/

enum {
	IDE_CMD_NOP						= 0x00,
	IDE_CMD_RESET					= 0x08,

	IDE_CMD_RECALIBRATE				= 0x10,
	IDE_CMD_EJECT_ATAPI				= 0x1b,

	IDE_CMD_READ_PIO				= 0x20,
	IDE_CMD_READ_PIO_UNSAFE			= 0x21, // no retry
	IDE_CMD_READ_PIO_ECC			= 0x22,
	IDE_CMD_READ_PIO_UNSAFE_ECC		= 0x23, // no retry
	IDE_CMD_READ_PIO_EXT			= 0x24,
	IDE_CMD_READ_DMA_EXT			= 0x25,
	IDE_CMD_READ_LOG_EXT			= 0x2f,

	IDE_CMD_WRITE_PIO				= 0x30,
	IDE_CMD_WRITE_PIO_UNSAFE		= 0x31, // no retry
	IDE_CMD_WRITE_PIO_ECC			= 0x32,
	IDE_CMD_WRITE_PIO_UNSAFE_ECC	= 0x33, // no retry
	IDE_CMD_WRITE_PIO_EXT			= 0x34,
	IDE_CMD_WRITE_DMA_EXT			= 0x35,

	IDE_CMD_WRITE_STREAM_DMA		= 0x3a,
	IDE_CMD_WRITE_STREAM_			= 0x3b,

	IDE_CMD_VERIFY					= 0x40,
	IDE_CMD_VERIFY_UNSAFE			= 0x40, // no retry
	IDE_CMD_READ_LOG_DMA			= 0x47,

	IDE_CMD_FORMAT					= 0x50, // don't fuck with
	IDE_CMD_CONFIGURE_STREAM		= 0x51,
	
	IDE_CMD_SEEK					= 0x70, // does nothing
	IDE_CMD_SET_DATE				= 0x77,

	IDE_CMD_RUN_DIAGNOSTIC			= 0x90,
	IDE_CMD_SET_GEOMETRY			= 0x91, // no touching
	IDE_CMD_READ_MICROCODE			= 0x92,
	IDE_CMD_READ_MICROCODE_DMA		= 0x93,

	IDE_CMD_PACKET					= 0xa0, // scsi
	IDE_CMD_IDENTIFY_PACKET			= 0xa1,
	IDE_CMD_READ_ATAPI				= 0xa8,
	IDE_CMD_WRITE_ATAPI				= 0xaa,
	IDE_CMD_READ_DMA				= 0xc8,
	IDE_CMD_READ_DMA_UNSAFE			= 0xc9, // no retry
	IDE_CMD_WRITE_DMA				= 0xca,
	IDE_CMD_WRITE_DMA_UNSAFE		= 0xcb, // no retry

	IDE_CMD_SMART					= 0xb0,
	IDE_CMD_CONFIGURE				= 0xb1,
	IDE_CMD_SANITISE				= 0xb4,

	IDE_CMD_CHECK_CARD				= 0xd1,
	IDE_CMD_MEDIA_SATUS				= 0xda,
	IDE_CMD_ACK_EJECTION			= 0xdb,
	IDE_CMD_LOCK_DOOR				= 0xde,
	IDE_CMD_UNLOCK_DOOR				= 0xdf,

	IDE_CMD_SHUTOFF					= 0xe0,
	IDE_CMD_IDLE					= 0xe1,
	IDE_CMD_SHUTOFF_TIMED			= 0xe2,
	IDE_CMD_IDLE_EVENTUALLY			= 0xe3, // take your time
	IDE_CMD_READ_BUFFER				= 0xe4,
	IDE_CMD_POWER_STATUS			= 0xe5,
	IDE_CMD_SLEEP					= 0xe6,
	IDE_CMD_WRITE_BUFFER			= 0xe8,
	IDE_CMD_WRITE_SAME				= 0xe9,
	IDE_CMD_IDENTIFY				= 0xec,
	IDE_CMD_WRITE_FEATURES			= 0xef,

	IDE_CMD_SET_PASSWORD			= 0xf1,
	IDE_CMD_UNLOCK					= 0xf2,
	IDE_CMD_PREPARE_ERASE			= 0xf3,
	IDE_CMD_ERASE					= 0xf4,
	IDE_CMD_FREEZE_LOCK				= 0xf5,
	IDE_CMD_DISABLE_PASSWORD		= 0xf6,
};

enum {
	IDE_IO_REG_DATA				= 0x00,
	IDE_IO_REG_ERROR			= 0x01,
	IDE_IO_REG_FEATURES			= 0x01,
	IDE_IO_REG_SECTOR_COUNT		= 0x02,
	IDE_IO_REG_LBA0				= 0x03,
	IDE_IO_REG_LBA1				= 0x04,
	IDE_IO_REG_LBA2				= 0x05,
	IDE_IO_REG_SELECT			= 0x06,
	IDE_IO_REG_COMMAND			= 0x07,
	IDE_IO_REG_STATUS			= 0x07,

	IDE_CTRL_REG_CTRL			= 0x00,
	IDE_CTRL_REG_ALT_STATUS		= 0x00,
	IDE_CTRL_REG_ADDRESS		= 0x01,
};

enum {
	IDE_ATA,
	IDE_ATAPI,
};

enum {
	IDE_MODE_CHS,
	IDE_MODE_LBA,
	IDE_MODE_LBA48,
};

enum {
	IDE_READ,
	IDE_WRITE,
};

#define IDE_DMA 0x70000

void ide_init();
int ide_ata_do_sector(ide_disk_t * disk, int direction, uint64_t lba, uint8_t sectors, void * buffer);
int ide_atapi_do_sector(ide_disk_t * disk, int direction, uint64_t lba, uint8_t sectors, void * buffer, uint16_t feature);