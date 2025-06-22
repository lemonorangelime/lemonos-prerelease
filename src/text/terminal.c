#include <stdint.h>
#include <graphics/graphics.h>
#include <cpuid.h>

int lock = 0;

int terminal_cprint(uint16_t * string, uint32_t colour) {
	if (!background.fb) {
		return 0;
	}
	int position = txt_string_draw(string, background.cursor.x, background.cursor.y, colour, &background);
	background.updated++;
	return 0;
}

void terminal_cputc(uint16_t character, uint32_t colour) {
	int ret = 1;
	while (ret) {
		ret = terminal_cprint((uint16_t[2]) {character, u'\0'}, colour);
	}
}

void terminal_print(uint16_t * string) {
	int ret = 1;
	while (ret) {
		ret = terminal_cprint(string, LEGACY_COLOUR_WHITE);
	}
}

void terminal_print8l(char * string, size_t len) {
	size_t i = 0;
	while (i < len && *string) {
		terminal_cputc(*string++, LEGACY_COLOUR_WHITE);
		i++;
	}
}

void terminal_putc(uint16_t character) {
	terminal_cputc(character, LEGACY_COLOUR_WHITE);
}
