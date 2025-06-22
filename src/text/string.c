#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

double rounders[] = {
	0.5,
	0.05,
	0.005,
	0.0005,
	0.00005,
	0.000005,
	0.0000005,
	0.00000005,
	0.000000005,
	0.0000000005,
	0.00000000005
};

// these work for ustr and char
int isdigit(int c) {
	return c >= '0' && c <= '9';
}

int isblank(int c) {
	return c == ' ' || c == '\t';
}

int isspace(int c) {
	return isblank(c) || c == '\f' || c == '\n' || c == '\r' || c == '\v';
}

long strtol(const char * str) {
	long acc = 0;
	int sign = 1;

	while(isspace(*str)) str++;

	if(*str == u'+') {
		str++;
	} 
	else if(*str == u'-') {
		sign = -1;
		str++;
	}

	while(*str && isdigit(*str)) {
		acc = acc * 10 + (*str - u'0');
		str++;
	}

	return sign > 0 ? acc : -acc;
}

void itoa(int val, char * buf, int base) {
	static char rbuf[64];
	char * ptr = rbuf;
	int neg = 0;

	if (val < 0) {
		neg = 1;
		val = -val;
	}

	if (val == 0) {
		*ptr++ = '0';
	}

	while (val) {
		int digit = (int)((long)val % (long)base);
		*ptr++ = digit < 10 ? (digit + '0') : (digit - 10 + 'a');
		val /= base;
	}

	if (neg) {
		*ptr++ = '-';
	}

	ptr--;

	while(ptr >= rbuf) {
		*buf++ = *ptr--;
	}

	*buf = 0;
}

long ustrtol(const uint16_t * str) {
	long acc = 0;
	int sign = 1;

	while(isspace(*str)) str++;

	if(*str == u'+') {
		str++;
	} 
	else if(*str == u'-') {
		sign = -1;
		str++;
	}

	while(*str && isdigit(*str)) {
		acc = acc * 10 + (*str - u'0');
		str++;
	}

	return sign > 0 ? acc : -acc;
}

int atoi(char * string) {
	return strtol(string);
}

int ustrtoi(uint16_t * string) {
	return ustrtol(string);
}

uint16_t * ustrnexti(uint16_t * string) {
	while (isdigit(*string)) {
		string++;
	}
	return string;
}

int strcmp(char * x, char * y) {
	int i = 0;
	while (x[i] != 0 && y[i] != 0) {
		if (x[i] != y[i]) {
			return 1;
		}
		i++;
	}
	return x[i] != 0 || y[i] != 0;
}

int ustrcmp(uint16_t * x, uint16_t * y) {
	int i = 0;
	while (x[i] != 0 && y[i] != 0) {
		if (x[i] != y[i]) {
			return 1;
		}
		i++;
	}
	return x[i] != 0 || y[i] != 0;
}

void ulldtoustr(uint64_t val, uint16_t * buf, int base) {
        static uint16_t rbuf[64];
        uint16_t * ptr = rbuf;
        if (val == 0) {
                *ptr++ = '0';
        }
        while (val) {
                int digit = (uint64_t)((uint64_t) val % (long) base);
                *ptr++ = digit < 10 ? (digit + '0') : (digit - 10 + 'a');
                val /= base;
        }
        ptr--;
        while (ptr >= rbuf) {
                *buf++ = *ptr--;
        }
        *buf = 0;
}

void ulldtoa(uint64_t val, char * buf, int base) {
        static char rbuf[64];
        char * ptr = rbuf;
        if (val == 0) {
                *ptr++ = '0';
        }
        while (val) {
                int digit = (uint64_t)((uint64_t) val % (long) base);
                *ptr++ = digit < 10 ? (digit + '0') : (digit - 10 + 'a');
                val /= base;
        }
        ptr--;
        while (ptr >= rbuf) {
                *buf++ = *ptr--;
        }
        *buf = 0;
}

void lldtoustr(int64_t val, uint16_t * buf, int base) {
        static uint16_t rbuf[64];
        uint16_t * ptr = rbuf;
        int neg = 0;
        if (val < 0) {
                neg = 1;
                val = -val;
        }
        if (val == 0) {
                *ptr++ = '0';
        }
        while (val) {
                int digit = (uint64_t)((uint64_t) val % (long) base);
                *ptr++ = digit < 10 ? (digit + '0') : (digit - 10 + 'a');
                val /= base;
        }
        if (neg) {
                *ptr++ = '-';
        }
        ptr--;
        while (ptr >= rbuf) {
                *buf++ = *ptr--;
        }
        *buf = 0;
}

/**
 *  stm32tpl --  STM32 C++ Template Peripheral Library
 *  Visit https://github.com/antongus/stm32tpl for new versions
 *
 *  Copyright (c) 2011-2020 Anton B. Gusev aka AHTOXA
 *
 *  file: ftoa.c
 *  functions: ftoa, ftoustr
 */

char * ftoa(double f, char * buf, int precision) {
	char * ptr = buf;
	char * p = ptr;
	char * p1;
	char c;
	long intPart;

	if (precision > 10) {
		precision = 10;
	}

	if (f < 0) {
		f = -f;
		*ptr++ = '-';
	}

	if (precision < 0) {
		*buf = 0;
		return buf;
	}
	if (precision) {
		f += rounders[precision];
	}
	intPart = f;
	f -= intPart;
	if (!intPart) {
		*ptr++ = '0';
	} else {
		p = ptr;
		while (intPart) {
			*p++ = '0' + intPart % 10;
			intPart /= 10;
		}
		p1 = p;
		while (p > ptr) {
			c = *--p;
			*p = *ptr;
			*ptr++ = c;
		}
		ptr = p1;
	}
	if (precision) {
		*ptr++ = '.';
		while (precision--) {
			f *= 10.0;
			c = f;
			*ptr++ = '0' + c;
			f -= c;
		}
	}
	*ptr = 0;
        ptr--;
        while (*ptr == '0') {
                *ptr = '\0';
                ptr--;
        }
        if (*ptr == '.') {
                *++ptr = '0';
        }
	return buf;
}

uint16_t * ftoustr(double f, uint16_t * buf, int precision) {
	uint16_t * ptr = buf;
	uint16_t * p = ptr;
	uint16_t * p1;
	uint16_t c;
	long intPart;

	if (precision > 10) {
		precision = 10;
	}

	if (f < 0) {
		f = -f;
		*ptr++ = u'-';
	}

	if (precision < 0) {
		*buf = 0;
		return buf;
	}
	if (precision) {
		f += rounders[precision];
	}
	intPart = f;
	f -= intPart;
	if (!intPart) {
		*ptr++ = u'0';
	} else {
		p = ptr;
		while (intPart) {
			*p++ = u'0' + intPart % 10;
			intPart /= 10;
		}
		p1 = p;
		while (p > ptr) {
			c = *--p;
			*p = *ptr;
			*ptr++ = c;
		}
		ptr = p1;
	}
	if (precision) {
		*ptr++ = u'.';
		while (precision--) {
			f *= 10.0;
			c = f;
			*ptr++ = u'0' + c;
			f -= c;
		}
	}
	*ptr = 0;
        ptr--;
        while (*ptr == u'0') {
                *ptr = u'\0';
                ptr--;
        }
        if (*ptr == u'.') {
                *++ptr = u'0';
        }
	return buf;
}

uint16_t * ustrcpy(uint16_t * dest, uint16_t * src) {
	return memcpy(dest, src, (ustrlen(src) * 2) + 2);
}

char * strcpy(char * dest, char * src) {
	return memcpy(dest, src, strlen(src) + 1);
}

uint16_t * atoustr(uint16_t * ustr, char * a) {
	uint16_t * old = ustr;
	while (*a) {
		*ustr = *a++;
		ustr++;
	}
	*ustr = 0;
	return old;
}

char * ustrtoa(char * a, uint16_t * ustr) {
	char * old = a;
	while (*ustr) {
		*a = *ustr++;
		a++;
	}
	*a = 0;
	return old;
}

uint16_t * ustrdup(uint16_t * string) {
	return ustrcpy(malloc((ustrlen(string) * 2) + 2), string);
}

uint16_t * toustrdup(char * string) {
	return atoustr(malloc((strlen(string) * 2) + 2), string);
}

char * strdup(char * string) {
	return strcpy(malloc(strlen(string) + 1), string);
}

int ustrlen(uint16_t * string) {
	int i = 0;
	while (*string++) {
		i++;
	}
	return i;
}

int strlen(const char * string) {
	int i = 0;
	while (*string++) {
		i++;
	}
	return i;
}

uint16_t * ustrcat(uint16_t * dst, uint16_t * src) {
	ustrcpy(dst + ustrlen(dst), src);
	return dst;
}

char * strcat(char * dst, char * src) {
	strcpy(dst + strlen(dst), src);
	return dst;
}

uint16_t * memset16(uint16_t * dest, uint16_t val, size_t length) {
	register uint16_t * temp = dest;
	while (length-- > 0) {
		*temp++ = val;
	}
	return dest;
}

void * memset(void * dest, int val, size_t length) {
	register uint8_t * temp = dest;
	while (length-- > 0) {
		*temp++ = val;
	}
	return dest;
}

void * memcpy96_8(void * dest, void * src, size_t len) {
	size_t remaining = (size_t) (len % 12);
	size_t size = (size_t) (len / 12);
	register long double * d = (long double *) dest;
	register long double * s = (long double *) src;
	while (size--) {
		*d++ = *s++;
	}
	memcpy((void *) d, (void *) s, remaining);
	return dest;
}

void * memcpy96_32(void * dest, void * src, size_t len) {
	size_t remaining = (size_t) (len % 3);
	size_t size = (size_t) (len / 3);
	register long double * d = (long double *) dest;
	register long double * s = (long double *) src;
	while (size--) {
		*d++ = *s++;
	}
	memcpy32((void *) d, (void *) s, remaining);
	return dest;
}

void * memcpy(void * dest, const void * src, size_t length) {
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
		: "0" (length/4), "q" (length),"1" ((long) dest),"2" ((long) src)
		: "memory"
	);
	return dest;
}

int memcmp(const void * str1, const void * str2, size_t count) {
	const unsigned char * s1 = (const unsigned char *) str1;
	const unsigned char * s2 = (const unsigned char *) str2;
	while (count-- > 0) {
		if (*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}
