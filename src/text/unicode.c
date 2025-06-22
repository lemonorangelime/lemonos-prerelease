// port of dolphinator unicode parser (yes thats the source of this)

#include <stdint.h>
#include <string.h>

uint8_t first_zero(uint8_t byte) {
	int i = 7;
	while ((byte >> i) & 1) {
		i--;
	}
	return 7 - i;
}

int utf8_strlen(char * string) {
	int size = strlen(string);
	int i = 0;
	int length = 0;
	while (i < size) {
		uint8_t chr = string[i];
		length++;
		if (!(chr & 0b10000000)) {
			i++;
			continue;
		}
		i += first_zero(chr);
	}
	return length;
}

void utf8toutf16l(char * in, uint16_t * out, int size) {
	int i = 0;
	while (i < size) {
		uint8_t chr = in[i];
		if (!(chr & 0b10000000)) {
			*out++ = chr;
			i++;
			continue;
		}
		uint8_t length = first_zero(chr);
		uint16_t codepoint = 0;
		while ((length--) && (i < size)) {
			uint8_t chr = in[i] & 0x3f;
			codepoint <<= 6;
			codepoint |= chr;
			i++;
		}
		*out++ = codepoint;
	}
	*out = 0;
}

void utf8toutf16(char * in, uint16_t * out) {
	utf8toutf16l(in, out, strlen(in));
}
