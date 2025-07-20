#include <drivers/audio/sb.h>
#include <cpuspeed.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <ports.h>
#include <util.h>
#include <pit.h>

static uint16_t sb_base = 0;

int sb_detect(uint16_t port) {
	outb(port + 0x06, 1);
	cpuspeed_wait_tsc(cpu_hz / 1000000);
	outb(port + 0x06, 0);
	cpuspeed_wait_tsc(cpu_hz / 1000);
	return inb(port + 0x0a) == 0xaa;
}

uint16_t sb_scan() {
	for (int i = 0; i <= 8; i++) {
		uint16_t port = 0x200 | (i << 4);
		if (sb_detect(port)) {
			return port;
		}
	}
	return 0;
}

void sb_init() {
	uint16_t port = sb_scan();
	if (port == 0) {
		return;
	}
	sb_base = port;
	printf(u"SB: detected Sound Blaster\n");
}