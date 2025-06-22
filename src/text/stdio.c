#include <stdint.h>
#include <string.h>
#include <terminal.h>
#include <stdarg.h>
#include <util.h>
#include <memory.h>
#include <input/input.h>
#include <multitasking.h>

uint32_t ansi_colour(int code) {
	uint32_t colours[] = { 0xff000000, 0xffcc0000, 0xff009a00, 0xffc4a000, 0xff0065a4, 0xff75507b, 0xff00989a, 0xffd0d0d0 };
	if (code < 30 || code > 37) {
		return 0;
	}
	code -= 30;
	return colours[code];
}

void ansi_handle_code(stdout_state_t * state, int * ansi_buffer, int length) {
	if (length < 1) {
		state->colour = 0xffffffff;
		return;
	}
	int code = *ansi_buffer;
	switch (code) {
		case 0:
			if (length < 2) {
				return;
			}
			uint32_t colour = ansi_colour(ansi_buffer[1]);
			if (!colour) {
				return;
			}
			state->colour = colour;
			return;
		case 38:
			if (length < 5 || ansi_buffer[1] != 2) {
				return;
			}
			uint8_t r = ansi_buffer[2];
			uint8_t g = ansi_buffer[2];
			uint8_t b = ansi_buffer[2];
			state->colour = 0xff000000 | (r << 16) | (g << 8) | b;
			return;
	}
}

void ansi_stdout_constructor(process_t * process) {
	stdout_state_t * state = malloc(sizeof(stdout_state_t));
	state->state = ANSI_STATE_PRINTING;
	state->idx = 0;
	state->colour = 0xffffffff;
	process->stdout_priv = state;
}

ssize_t ansi_stdout_print(stdout_state_t * state, uint16_t * buffer, size_t size) {
	// we need a proper ANSI decoder
	while (size--) {
		uint16_t chr = *buffer;
		switch (state->state) {
			case ANSI_STATE_PRINTING:
				if (chr == 0x1b) {
					state->state = ANSI_STATE_ESCAPE;
					break;
				}
				terminal_cputc(chr, state->colour);
				break;
			case ANSI_STATE_ESCAPE:
				if (chr == u'[') {
					state->state = ANSI_STATE_BRACKET;
					break;
				}
				state->state = ANSI_STATE_PRINTING;
				break;
			case ANSI_STATE_BRACKET:
				state->idx = 0;
				state->state = ANSI_STATE_VALUE;
				break;
			case ANSI_STATE_VALUE:
				state->buffer[state->idx] = ustrtol(buffer);
				buffer = ustrnexti(buffer) - 1;
				state->idx++;
				state->state = ANSI_STATE_END;
				break;
			case ANSI_STATE_END:
				if (chr == ';') {
					state->state = ANSI_STATE_VALUE;
				} else if (chr == 'm') {
					ansi_handle_code(state, state->buffer, state->idx);
					state->state = ANSI_STATE_PRINTING;
				}
				break;
		}
		buffer++;
	}
	return 0;
}

ssize_t ansi_stdout_handler(process_t * process, uint16_t * buffer, size_t size) {
	// we need a proper ANSI decoder
	stdout_state_t * state = process->stdout_priv;
	ansi_stdout_print(state, buffer, size);
}

void hook_stdout(process_t * process, stdout_handler_t handler, stdout_constructor_t constructor) {
	process->stdout_handler = handler;
	constructor(process);
}

void printf(uint16_t * fmt, ...) {
	int save = interrupts_enabled();
	disable_interrupts();
	va_list listp;
	va_list * argv;
	uint16_t c;
	uint8_t ub;
	unsigned long ul = 0;
	uint64_t ull = 0;
	long l = 0;
	// double f = 0;
	uint16_t * p;
	char * str;
	uint16_t buffer[128];
	unsigned char * buffer8;
	va_start(listp, fmt);
	argv = &listp;
	memset16(buffer, 0, 128);
	while ((c = *fmt) != u'\0') {
		if (c != u'%') {
			terminal_putc(c);
			fmt++;
			continue;
		} else {
			fmt++;
			c = *fmt;
		if (c == u'0') {
			break;
		}
		switch (c) {
			default:
				break;
			case u'x':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 16);
				terminal_print(buffer);
				break;
			case u'b':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 2);
				terminal_print(u"0b");
				terminal_print(buffer);
				break;
			case u'r':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 16);
				ul = ustrlen(buffer);
				ul = 8 - ul;
				terminal_print(u"0x");
				while (ul--) {
					terminal_print(u"0");
				}
				terminal_print(buffer);
				break;
			case u'R':
				ull = *(uint64_t *) va_arg(*argv, unsigned long);
				ulldtoustr(ull, buffer, 16);
				ul = ustrlen(buffer);
				ul = 16 - ul;
				terminal_print(u"0x");
				while (ul--) {
					terminal_print(u"0");
				}
				terminal_print(buffer);
				break;
			case u'w':
				ul = (uint16_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 16);
				ul = ustrlen(buffer);
				ul = 4 - ul;
				while (ul--) {
					terminal_print(u"0");
				}
				terminal_print(buffer);
				break;
			case u'B':
				ub = (uint8_t) va_arg(*argv, unsigned long);
				ulldtoustr(ub, buffer, 16);
				ub = ustrlen(buffer);
				ub = 2 - ub;
				terminal_print(u"0x");
				while (ub--) {
					terminal_print(u"0");
				}
				terminal_print(buffer);
				break;
			case u'f': /*
				f = (double) va_arg(*argv, double);
				ftoustr(f, buffer, 10);
				terminal_print(buffer); */
				break;
			case u'd':
				l = (int32_t) va_arg(*argv, long);
				lldtoustr(l, buffer, 10);
				terminal_print(buffer);
				break;
			case u'o':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 8);
				terminal_print(buffer);
				break;
			case u'u':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 10);
				terminal_print(buffer);
				break;
			case u'l':
				ull = *(uint64_t *) va_arg(*argv, uint64_t *);
				ulldtoustr(ull, buffer, 10);
				terminal_print(buffer);
				break;
			case u's':
				p = (uint16_t *) va_arg(*argv, uint16_t *);
				terminal_print(p);
				break;
			case u'8':
				str = (char *) va_arg(*argv, char *);
				p = malloc((strlen(str) + 1) * 2);
				atoustr(p, str);
				terminal_print(p);
				free(p);
				break;
			case u'c':
				ul = (uint16_t) va_arg(*argv, uint16_t);
				terminal_putc(ul);
				break;
			case u'%':
				terminal_putc(u'%');
				break;
		}
		fmt++;
		continue;
		}
	}
	va_end(listp);
	interrupts_restore(save);
}

void cprintf(uint32_t colour, uint16_t * fmt, ...) {
	int save = interrupts_enabled();
	disable_interrupts();
	va_list listp;
	va_list * argv;
	uint16_t c;
	uint8_t ub;
	unsigned long ul = 0;
	uint64_t ull = 0;
	signed long l = 0;
	// double f = 0;
	uint16_t * p;
	char * str;
	uint16_t buffer[128];
	unsigned char * buffer8;
	va_start(listp, fmt);
	argv = &listp;
	memset16(buffer, 0, 128);
	while ((c = *fmt) != u'\0') {
		if (c != u'%') {
			terminal_cputc(c, colour);
			fmt++;
			continue;
		} else {
			fmt++;
			c = *fmt;
		if (c == u'0') {
			break;
		}
		switch (c) {
			default:
				break;
			case u'x':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 16);
				terminal_cprint(buffer, colour);
				break;
			case u'r':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 16);
				ul = ustrlen(buffer);
				ul = 8 - ul;
				terminal_cprint(u"0x", colour);
				while (ul--) {
					terminal_cprint(u"0", colour);
				}
				terminal_cprint(buffer, colour);
				break;
			case u'w':
				ul = (uint16_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 16);
				ul = ustrlen(buffer);
				ul = 4 - ul;
				while (ul--) {
					terminal_cprint(u"0", colour);
				}
				terminal_cprint(buffer, colour);
				break;
			case u'B':
				ub = (uint8_t) va_arg(*argv, unsigned long);
				ulldtoustr(ub, buffer, 16);
				ub = ustrlen(buffer);
				ub = 2 - ub;
				terminal_cprint(u"0x", colour);
				while (ub--) {
					terminal_cprint(u"0", colour);
				}
				terminal_cprint(buffer, colour);
				break;
			case u'b':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 2);
				terminal_cprint(u"0b", colour);
				terminal_cprint(buffer, colour);
				break;
			case u'f': /*
				f = (double) va_arg(*argv, double);
				ftoustr(f, buffer, 10);
				terminal_cprint(buffer, colour); */
				break;
			case u'd':
				l = (int32_t) va_arg(*argv, long);
				lldtoustr(l, buffer, 10);
				terminal_cprint(buffer, colour);
				break;
			case u'o':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 8);
				terminal_cprint(buffer, colour);
				break;
			case u'u':
				ul = (uint32_t) va_arg(*argv, unsigned long);
				ulldtoustr(ul, buffer, 10);
				terminal_cprint(buffer, colour);
				break;
			case u'l':
				ull = *(uint64_t *) va_arg(*argv, uint64_t *);
				ulldtoustr(ull, buffer, 10);
				terminal_cprint(buffer, colour);
				break;
			case u's':
				p = (uint16_t *) va_arg(*argv, uint16_t *);
				terminal_cprint(p, colour);
				break;
			case u'8':
				str = (char *) va_arg(*argv, char *);
				p = malloc((strlen(str) + 1) * 2);
				atoustr(p, str);
				terminal_cprint(p, colour);
				free(p);
				break;
			case u'c':
				ul = (uint16_t) va_arg(*argv, uint16_t);
				terminal_cputc(ul, colour);
				break;
			case u'%':
				terminal_cputc(u'%', colour);
				break;
		}
		fmt++;
		continue;
		}
	}
	va_end(listp);
	interrupts_restore(save);
}
