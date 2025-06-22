#pragma once

#include <stdint.h>

enum {
	CMOS_RTC_SECONDS = 0,
	CMOS_RTC_SECOND_ALARM,
	CMOS_RTC_MINUTES,
	CMOS_RTC_MINUTE_ALARM,
	CMOS_RTC_HOURS,
	CMOS_RTC_HOUR_ALARM,
	CMOS_RTC_DAY,
	CMOS_RTC_WEEK,
	CMOS_RTC_MONTH,
	CMOS_RTC_YEAR,
	CMOS_STAT_A,
	CMOS_STAT_B,
	CMOS_STAT_C,
	CMOS_STAT_D,
	CMOS_DIAGNOSTIC,
	CMOS_SHUTDOWN_STATUS,
	CMOS_FLOPPY,
	CMOS_HDA,
	CMOS_EQUIPMENT = 0x14,
	CMOS_CENTURY = 0x32, // every other register is seemingly bios specific
	CMOS_SERIAL = 0x40, // 0x40 - 0x47
	CMOS_CTRL_A = 0x4a,
	CMOS_CTRL_B = 0x4b,
};

enum {
	CMOS_PORT_ADDR = 0x70,
	CMOS_PORT_DATA = 0x71,
};

uint8_t cmos_read_register(uint16_t reg);
void cmos_write_register(uint16_t reg, uint8_t value);