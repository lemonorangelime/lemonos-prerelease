#pragma once

#include <stdint.h>

typedef struct {
	uint32_t size;
	uint32_t version;
	uint32_t type;
	int32_t response;
	uint32_t padding;
	uint32_t padding1;
} __attribute__((packed)) vbox_header_t;

typedef struct {
	vbox_header_t header;
	uint32_t version;
	uint32_t ostype;
} __attribute__((packed)) vbox_guest_info_packet_t;

typedef struct {
	vbox_header_t header;
	uint32_t caps;
} __attribute__((packed)) vbox_guest_caps_packet_t;

typedef struct {
	vbox_header_t header;
	uint32_t events;
} __attribute__((packed)) vbox_ack_packet_t;

typedef struct {
	vbox_header_t header;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t ack;
} __attribute__((packed)) vbox_display_update_packet_t;

typedef struct {
	vbox_header_t header;
	uint32_t flags;
	int32_t x;
	int32_t y;
} __attribute__((packed)) vbox_mouse_packet_t;

typedef struct {
	uint32_t idk;
	uint32_t idk_either;
	uint32_t irq_events;
	uint32_t irq_mask;
} __attribute__((packed)) vbox_vmm_t;

enum {
	VBOX_GET_MOUSE				= 1,
	VBOX_SET_MOUSE				= 2,
	VBOX_ACK					= 41,
	VBOX_REPORT_INFO			= 50,
	VBOX_GET_DISPLAY			= 51,
	VBOX_REPORT_CAPABILITIES	= 55,
};

enum {
	VBOX_OSTYPE_UNKNOWN = 0x00001000,
};

enum {
	VBOX_CAPS_SEAMLESS = 0b00000001,
	VBOX_CAPS_WINDOWS  = 0b00000010,
	VBOX_CAPS_GRAPHICS = 0b00000100,
};

enum {
	VBOX_EVENT_DISPLAY	= 0b0000000100,
	VBOX_EVENT_MOUSE	= 0b1000000000,
};

enum {
	VBOX_MOUSE_ENABLE  = 0b00010001,
	VBOX_MOUSE_DISABLE = 0b00000000,
};

#define VBOX_HEADER_VERSION 0x10001
#define VBOX_VERSION 0x10003

void vbox_init();