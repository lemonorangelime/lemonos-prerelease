#pragma once

#include <stdint.h>
#include <power/perf.h>

int isdigit(int c);
int isblank(int c);
int isspace(int c);
uint16_t * ustrnexti(uint16_t * string);
int atoi(char * string);
void itoa(int val, char * buf, int base);
int ustrlen(uint16_t * string);
int strlen(const char * string);
int ustrcmp(uint16_t * x, uint16_t * y);
int strcmp(char * x, char * y);
char * ustrtoa(char * a, uint16_t * ustr);
char * strcat(char * dst, char * src);
uint16_t * ustrcat(uint16_t * dst, uint16_t * src);
int memcmp(const void * str1, const void * str2, size_t count);
uint16_t * ustrcpy(uint16_t * dest, uint16_t * src);
char * strcpy(char * dest, char * src);
uint16_t * ustrdup(uint16_t * string);
char * strdup(char * string);
uint16_t * atoustr(uint16_t * ustr, char * a);
uint16_t * toustrdup(char * string);

void * memcpy(void * dest, const void * src, size_t length);

uint16_t * memset16(uint16_t * dest, uint16_t val, size_t length);
void * memset(void * dest, int val, size_t length);

void ulldtoustr(uint64_t val, uint16_t * buf, int base);
void ulldtoa(uint64_t val, char * buf, int base);
void lldtoustr(int64_t val, uint16_t * buf, int base);
long ustrtol(const uint16_t * str);

uint16_t * ftoustr(double f, uint16_t * buf, int precision);
char * ftoa(double f, char * buf, int precision);
int strlen(const char * string);

/*
inline uint32_t * memset32(uint32_t * dest, uint32_t val, size_t length) {
	register uint32_t * temp = dest;
	while (length-- > 0) {
		*temp++ = val;
	}
	return dest;
}*/

inline uint32_t * memset32(uint32_t * dest, uint32_t val, size_t length) {
	void * temp = dest;
	__asm__ volatile (
		"rep stosl"
		: "=D"(dest),"=c"(length)
		: "0"(dest),"a"(val),"1"(length)
		: "memory"
	);
	return temp;
}


inline uint32_t * memcpy32(uint32_t * dest, uint32_t * src, size_t length) {
	int d0, d1, d2;
	asm volatile (
		"rep ; movsl\n\t"
		"testb $2,%b4\n\t"
		"je 1f\n\t"
		"movsw\n"
		"1:\ttestb $1,%b4\n\t"
		"je 2f\n\t"
		"movsb\n"
		"2:"

		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		: "0" (length), "q" (length*4),"1" ((long) dest),"2" ((long) src)
		: "memory"
	);
	return dest;
}
