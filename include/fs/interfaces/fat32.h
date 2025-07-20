#pragma once

#include <stdint.h>

typedef struct {
	uint8_t bootjump[3];
	char identifier[8];
	uint16_t sector_size;
	uint8_t cluster_size;
	uint16_t reserved_sectors;
	uint8_t tables;
	uint16_t root_entries;
	uint16_t total_sectors;
	uint8_t type;
	uint16_t sector_per_fat;
	uint16_t sector_per_track;
	uint16_t heads;
	uint32_t hidden_sectors;
	uint32_t large_sectors;

	uint32_t sector_per_fat32;
	uint16_t flags;
	uint16_t version;
	uint32_t root_cluster;
	uint16_t fsinfo_sector;
	uint16_t backup_sector;
	char reserved[12];
	uint8_t drive_number;
	uint8_t nt_flags;
	uint8_t signature;
	uint32_t serial;
	char volume_label[11];
	char system_identifier[8];
	char bootcode[420];
	uint16_t bootable_signature;
} __attribute__((packed)) fat32_t;

typedef struct {
	uint8_t bootjump[3];
	char identifier[8];
	uint16_t sector_size;
	uint8_t cluster_size;
	uint16_t reserved_sectors;
	uint8_t tables;
	uint16_t root_entries;
	uint16_t total_sectors;
	uint8_t type;
	uint16_t sector_per_fat;
	uint16_t sector_per_track;
	uint16_t heads;
	uint32_t hidden_sectors;
	uint32_t large_sectors;

	uint8_t drive;
	uint8_t flags;
	uint8_t signature;
	uint32_t serial;
	char label[11];
	char system_identifier[8];
	char bootcode[448];
	uint16_t bootable_signature;
} __attribute__((packed)) fat16_t;

typedef struct {
	uint16_t hour : 5;
	uint16_t minutes : 6;
	uint16_t seconds : 5;
} __attribute__((packed)) fat32_creation_time_t;

typedef struct {
	uint16_t year : 7;
	uint16_t month : 4;
	uint16_t day : 5;
} __attribute__((packed)) fat32_creation_date_t;

typedef struct {
	char filename[11];
	uint8_t attributes;
	uint8_t reserved;
	uint8_t creation_centisecond;
	fat32_creation_time_t creation_time;
	fat32_creation_date_t creation_date;
	fat32_creation_date_t access_date;
	uint16_t root_cluster_high;
	fat32_creation_time_t modification_time;
	fat32_creation_date_t modification_date;
	uint16_t root_cluster_low;
	uint32_t size;
} __attribute__((packed)) fat32_directory_t;

typedef struct {
	uint8_t order;
	uint16_t first_chunk[5];
	uint8_t attributes;
	uint8_t type;
	uint8_t checksum;
	uint16_t second_chunk[6];
	uint16_t zero;
	uint16_t third_chunk[2];
} __attribute__((packed)) fat32_long_name_t;

enum {
	FAT_ATTRIBUTES_RDONLY           = 0b0000001,
	FAT_ATTRIBUTES_HIDDEN           = 0b0000010,
	FAT_ATTRIBUTES_SYSTEM           = 0b0000100,
	FAT_ATTRIBUTES_VOLUME           = 0b0001000,
	FAT_ATTRIBUTES_DIRECTORY        = 0b0010000,
	FAT_ATTRIBUTES_ARCHIVE          = 0b0100000,
	FAT_ATTRIBUTES_DEVICE           = 0b1000000,
	FAT_ATTRIBUTES_LONG             = 0b0001111,
};

void fat_init();