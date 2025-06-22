#pragma once

#include <stdint.h>

enum {
	ET_NONE = 0,
	ET_REL,
	ET_EXEC,
	ET_DYN,
	ET_CORE,
};

enum {
	// (ignored value not relevant to us.)
	ET_UNSPECIFIED = 0,
	ET_I386 = 0x03,
	ET_AMD64 = 0x3e, // extend later...
};

enum {
	ABI_UNIX = 0,
};

enum {
	EI_CLASS_I386 = 1,
	EI_CLASS_AMD64,
};

enum {
	EI_DATA_LITTLE = 1,
	EI_DATA_BIG,
};

typedef struct {
	uint32_t padding;
	uint32_t offset;
} __attribute__((packed)) elf_ph_t;

typedef struct {
	char ident[4];
	char class;
	char data;
	char version;
	char abi;
	char pad[8];
	uint16_t type;
	uint16_t machine;
	char pad2[4];
	uint32_t entry;
	uint32_t ph_off;
} __attribute__((packed)) elf_t;

int exec_initrd(char * filename, int argc, char * argv[]);