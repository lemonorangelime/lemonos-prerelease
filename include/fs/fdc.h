#pragma once

#include <stdint.h>

typedef struct {
	int id;
	int type;
	int tracksectors;
	int size;
} floppy_t;

enum {
	FDC_PORT_OUT = 2,
	FDC_PORT_STATUS = 4,
	FDC_PORT_FIFO = 5,
	FDC_PORT_CONF = 7,
};

enum {
	FDC_CMD_SELECT = 3,
	FDC_CMD_DETECT = 4,
	FDC_CMD_WRITE = 5,
	FDC_CMD_READ = 6,
	FDC_CMD_CALIBRATE = 7,
	FDC_CMD_SENSE = 8,
	FDC_CMD_SEEK = 15,
	FDC_CMD_IDENTIFY = 16,
	FDC_CMD_PERPENDICULAR = 18,
	FDC_CMD_CONF = 19,
};

enum {
	FDC_NONE = 0,
	FDC_360KB,
	FDC_1_2MB,
	FDC_720KB,
	FDC_1_44MB,
	FDC_2_88MB,
};

enum {
	FDC_500KBPS = 0,
	FDC_300KBPS,
	FDC_250KBPS,
	FDC_1MBPS,
};

enum {
	FDC_STATUS_READY    = 0b10000000,
	FDC_STATUS_FLIGHT   = 0b01000000,
	FDC_STATUS_DMA      = 0b00100000,
	FDC_STATUS_BUSY     = 0b00010000,
	FDC_STATUS_D4_SEEK  = 0b00001000,
	FDC_STATUS_D3_SEEK  = 0b00000100,
	FDC_STATUS_D2_SEEK  = 0b00000010,
	FDC_STATUS_D1_SEEK  = 0b00000001,
};

enum {
	FDC_OUT_MOTOR4  = 0b10000000,
	FDC_OUT_MOTOR3  = 0b01000000,
	FDC_OUT_MOTOR2  = 0b00100000,
	FDC_OUT_MOTOR1  = 0b00010000,
	FDC_OUT_IRQ     = 0b00001000,
	FDC_OUT_RESET   = 0b00000100,
	FDC_OUT_SELECT1 = 0b00000010,
	FDC_OUT_SELECT0 = 0b00000001,
};

enum {
	FDC_CONF_AUTOSEEK = 0b00010000,
	FDC_CONF_NOFIFO   = 0b00100000,
	FDC_CONF_NOPOLL   = 0b01000000,
};

enum {
	FDC_SECTOR_SIZE = 512,
};

#define FDC_PORT 0x3f0
#define FDC_IRQ 38
#define FDC_DMA 0x10000
#define FDC_DMA_SIZE 0xffff

int fdc_datarate(int type);
int fdc_tracksectors(int type);

void fdc_outb(int port, uint8_t data);
uint8_t fdc_inb(int port);
void fdc_wait();
void fdc_dma(int direction, uint16_t count);
int fdc_track(int disk, int cylinder, int head, int direction, void * buffer);
int fdc_reset(int disks);
int fdc_calibrate(int disk);
int fdc_seek(int disk, int cylinder, int head);
void fdc_motor(int disk, int power);
uint16_t fdc_sense();
int fdc_cmd(int cmd);
uint8_t fdc_read();
void fdc_init();