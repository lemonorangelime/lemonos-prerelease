#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <multitasking.h>
#include <drivers/rng/tpm/tpmdev.h>
#include <drivers/rng/tpm/tpm.h>
#include <util.h>
#include <pit.h>
#include <drivers/rng/rng.h>

// dont look at this too closely it's in dire need of cleaning up

uint32_t tpm_inb(uint32_t address) {
	return *(volatile uint8_t *) (TPM_ADDRESS | address);
}

uint32_t tpm_inw(uint32_t address) {
	return *(volatile uint16_t *) (TPM_ADDRESS | address);
}

uint32_t tpm_ind(uint32_t address) {
	return *(volatile uint32_t *) (TPM_ADDRESS | address);
}

uint32_t tpm_outb(uint32_t address, uint8_t data) {
	*(volatile uint8_t *) (TPM_ADDRESS | address) = data;
}

uint32_t tpm_outw(uint32_t address, uint16_t data) {
	*(volatile uint16_t *) (TPM_ADDRESS | address) = data;
}

uint32_t tpm_outd(uint32_t address, uint32_t data) {
	*(volatile uint32_t *) (TPM_ADDRESS | address) = data;
}

void tpm_stall_for(uint8_t mask, uint8_t result) {
	while (tpm_inb(TPM_REG_STATUS) & mask != result) {}
}

void tpm_stall_for_any(uint8_t mask) {
	while (tpm_inb(TPM_REG_STATUS) & mask == 0) {}
}

int tpm_check_status(uint8_t mask) {
	return (tpm_inb(TPM_REG_STATUS) & mask) != mask;
}

int tpm_check_status_any(uint8_t mask) {
	return (tpm_inb(TPM_REG_STATUS) & mask) != 0;
}

void tpm_write(void * buffer, int size) {
	uint8_t * b = buffer;
	int burst = 0;
	tpm_outb(TPM_REG_STATUS, TPM_STATUS_STARTCMD);
	while (size > 0) {
		burst = tpm_inb(TPM_REG_STATUS + 1);
		burst += tpm_inb(TPM_REG_STATUS + 2) << 8;

		if (burst == 0) {
			continue;
		}

		while ((burst--) && (size--) > 0) {
			tpm_outb(TPM_REG_DATA_FIFO, *b++);
		}
	}
	tpm_outb(TPM_REG_STATUS, TPM_STATUS_EXECUTE);
}

void tpm_write_noexec(void * buffer, int size) {
	uint8_t * b = buffer;
	int burst = 0;
	while (size > 0) {
		burst = tpm_inb(TPM_REG_STATUS + 1);
		burst += tpm_inb(TPM_REG_STATUS + 2) << 8;

		if (burst == 0) {
			continue;
		}

		while ((burst--) && (size--) > 0) {
			tpm_outb(TPM_REG_DATA_FIFO, *b++);
		}
	}
}


int tpm_read_data(void * buffer, int size) {
	uint8_t * buf = buffer;
	int burst = 0;
	int read = 0;
	while (size > 0 && tpm_check_status_any(TPM_STATUS_AVAILABLE | TPM_STATUS_VALID)) {
		burst = tpm_inb(TPM_REG_STATUS + 1);
		burst += tpm_inb(TPM_REG_STATUS + 2) << 8;

		if (burst == 0) {
			continue;
		}

		while ((burst-- > 0) && (size-- > 0)) {
			*buf++ = tpm_inb(TPM_REG_DATA_FIFO);
			read++;
		}
	}
	return read;
}

int tpm_read(void * buffer, int size) {
	uint8_t * buf = buffer;
	int i = tpm_read_data(buffer, 6);
	if (i != 6) {
		return -1;
	}
	size -= 6;
	buffer += 6;

	int length = (buf[5] | (buf[4] << 8)) - 6;
	i += tpm_read_data(buffer, length);
	if (i != (6 + length)) {
		return -1;
	}

	tpm_outb(TPM_REG_STATUS, TPM_STATUS_STARTCMD);
	return i;
}

int tpm_wait() {
	uint64_t target = ticks + pit_freq;
	while (ticks < target) {
		yield();
	}
	return 0;
}

void * tpm_cmd(uint16_t cmd, void * buffer, int size) {
	uint8_t cmdbuf[] = {0x80, 0x01, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff};
	uint16_t all = size + 10; // size + tag
	cmdbuf[8] = cmd >> 8; // write command ordinal (their name not mine)
	cmdbuf[9] = cmd & 0xff;
	
	cmdbuf[4] = all >> 8; // write size
	cmdbuf[5] = all & 0xff;

	tpm_outb(TPM_REG_STATUS, TPM_STATUS_STARTCMD); // tell it we want to write a command
	tpm_write_noexec(cmdbuf, 10); // write tag
	if (buffer && size != 0) {
		tpm_write_noexec(buffer, size); // and buffer
	}
	tpm_outb(TPM_REG_STATUS, TPM_STATUS_EXECUTE); // then execute
	return buffer;
}

void * tpm_rng(void * buffer, int size) {
	uint16_t requested = htonw(size);
	tpm_cmd(TPM_CMD_GETRAND, &requested, 2);
	tpm_wait();

	uint8_t tag[12];
	int i = tpm_read_data(tag, 12);
	if (i != 12) {
		return NULL;
	}

	i = tpm_read_data(buffer, size);
	if (i != size) {
		return NULL;
	}
	tpm_outb(TPM_REG_STATUS, TPM_STATUS_STARTCMD);
	return buffer;
}

int tpm_hash(uint16_t algorithm, void * in, int insize, void * out, int outsize) {
	uint8_t * cmdbuf = malloc(insize + 8);
	uint8_t owner[] = {0x40, 0x00, 0x00, 0x01};
	memcpy(&cmdbuf[4 + insize], owner, 4);
	cmdbuf[0] = insize >> 8; // size
	cmdbuf[1] = insize & 0xff;
	memcpy(&cmdbuf[2], in, insize); // message
	cmdbuf[insize + 2] = algorithm >> 8; // size
	cmdbuf[insize + 3] = algorithm & 0xff;
	// 6F C5 16 96 79 E7 B2 ED 2F A8 E8 41 4D260A 38 58 FB 15 D4 4E 31 54 C8 A6 4C 6E BB 0283FB 6B 6F 42 2B 36 A7 B5 A4 E4 B2B5 E7 64 C172D5C77D 40 76 CA E2B976FB138E6C 56 A671B9

	tpm_cmd(TPM_CMD_HASH, cmdbuf, insize + 8);
	tpm_wait();

	uint8_t tag[12];
	int i = tpm_read_data(tag, 12);
	if (i != 12) {
		return -1;
	}
	int size = (tag[11] << 8) | tag[10];
	size = size > outsize ? outsize : size;

	i = tpm_read_data(out, size);
	if (i != size) {
		return -1;
	}
	tpm_outb(TPM_REG_STATUS, TPM_STATUS_STARTCMD);
	return size;
}

uint64_t tpm_get_time() {
	tpm_cmd(TPM_CMD_GETTIME, NULL, 0);
	tpm_wait();

	int size = 35;
	uint8_t response[size];
	tpm_read(response, size);

	uint64_t time = *(uint64_t *) &response[12];
	return htonq(time);
}

size_t tpm_rng_generate(rng_backend_t * backend, void * buffer, size_t size) {
	return (tpm_rng(buffer, size) == NULL) ? 0 : size;
}

size_t tpm_rng_seed(rng_backend_t * backend, void * buffer, size_t size) {
	return 0;
}

void tpm_init() {
	uint32_t viddid = tpm_ind(TPM_REG_VID);
	if (viddid == 0 || viddid == 0xffffffff) {
		return;
	}
	printf(u"TPM device found [%w:%w]\n", viddid & 0xffff, (viddid >> 16) & 0xffff);

	rng_backend_t * backend = rng_create_backend(u"TPM", tpm_rng_generate, tpm_rng_seed);
	rng_register_backend(backend);
}
