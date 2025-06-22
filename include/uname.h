#pragma once

typedef struct {
	char sysname[64 + 1];
	char nodename[64 + 1];
	char release[64 + 1];
	char version[64 + 1];
	char machine[64 + 1];
	char domainname[64 + 1];
} uname_t;

void uname(uname_t * name);
void uname_init();