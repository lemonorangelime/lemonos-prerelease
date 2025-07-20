/* ughhh so many globals this is awful also, this has spent *way* too long
   being handled in kernel space, I haven't moved this into a seperate binary
   simply out of laziness and convience but it ***really*** needs to be done
   at some point */

#include <graphics/graphics.h>
#include <graphics/displays.h>
#include <input/input.h>
#include <multitasking.h>
#include <multiboot.h>
#include <memory.h>
#include <graphics/font.h>
#include <string.h>
#include <linked.h>
#include <stdio.h>
#include <drivers/input/mouse.h>
#include <pit.h>
#include <panic.h>
#include <ports.h>
#include <assert.h>
#include <util.h>
#include <sysrq.h>
#include <bin_formats/elf.h>
#include <bus/pci.h>

int legacy_colour[16] = {
        0x000000,
        0x0000aa,
        0x00aa00,
        0x00aaaa,
        0xaa0000,
        0xaa00aa,
        0xaa5500,
        0xaaaaaa,
        0x555555,
        0x5555ff,
        0x55ff55,
        0x55ffff,
        0xff5555,
        0xff55ff,
        0xffff55,
        0xffffff
};

rect_2d_t root_window;
rect_2d_t back_buffer;
rect_2d_t background;
rect_2d_t taskbar;
rect_2d_t cursor;
rect_2d_t startmenu;

taskbar_button_t * startbutton;

window_t fake_root_window; // fake root_window window

window_t * active_window;

linked_t * taskbar_buttons;
linked_t * windows;
uint32_t button_offset;
uint32_t button_border = 0xff000000;
int taskbar_height = 28;
int taskbar_updated = 0;
int taskbar_button_count = 0;
int window_count = 0;

uint32_t background_colour = 0xff008080; // 0xff5500aa;
uint32_t taskbar_colour = 0xffaaaaaa;
uint32_t taskbar_highlight_colour = 0xffffffff;
uint32_t tertiary_colour = 0xff000080;
uint32_t window_background = 0xffaaaaaa;

int gfx_init_done = 0;
int fps = 0;
int locked_fps = 1;
int gfx_one_fb = 0;

static int old_mouse_x = 0;
static int old_mouse_y = 0;
static int resize_after_frame = 0;
static int resize_width = 0;
static int resize_height = 0;

int cursor_disabled = 0;

int taskbar_y_offset = 0;
int taskbar_size = 0;
static int back_buffer_size = 0;
static int background_size = 0;

inline uint32_t font_get_character(uint16_t character) {
	for (uint32_t i = 0; i < font_size; i++) {
		if (((uint16_t) font[i][0]) == character) {
			return i;
		}
	}
	return font_size - 1;
}

void font_truecolour_draw(uint32_t * fb, uint32_t chr, uint32_t colour, uint32_t position) {
	if (chr & 0xff000000) {
		fb[position] = chr | 0xff000000;
	}
}

void font_legacy_draw(uint32_t * fb, uint32_t chr, uint32_t colour, uint32_t position) {
	if (chr == 1) {
		fb[position] = colour | 0xff000000;
	} else if (chr > 1) {
		fb[position] = legacy_colour[chr] | 0xff000000;
	}
}

void font_combining_draw(uint32_t * fb, uint32_t chr, uint32_t colour, uint32_t position) {
	font_legacy_draw(fb, chr, colour, position - 8);
}

void font_blank_draw(uint32_t * fb, uint32_t chr, uint32_t colour, uint32_t position) {
	fb[position] = 0;
}

inline font_drawer_t font_get_drawer(uint32_t chr) {
	switch (font[chr][1]) {
		case FONT_LEGACY:
				return font_legacy_draw;
		case FONT_BLANK:
				return font_blank_draw;
		case FONT_COMBINING:
				return font_combining_draw;
		case FONT_TRUECOLOUR:
				return font_truecolour_draw;
	}
	return font_legacy_draw;
}

void gfx_char_draw_inline(uint16_t character, int x, int y, uint32_t colour, rect_2d_t * rect) {
	uint32_t position = ((y * 16) * rect->size.width) + (x * 8);
	uint32_t chr = font_get_character(character); // cant call it char, GRAHHHH
	font_drawer_t drawer = font_get_drawer(chr);
	int line = (y * 16);
	int pixel = 0;
	for (int i = 0; i < 128; i++) {
		if (pixel == 8) {
			line++; // go to the next line
			position = (line * rect->size.width) + (x * 8);
			pixel = 0; // go back to the start of the line
		}
		if (position > (rect->size.height * rect->size.width)) {
			continue;
		}
		drawer(rect->fb, font[chr][i + 2], colour, position + pixel);
		pixel++;
	}
}

void gfx_char_draw(uint16_t character, int x, int y, uint32_t colour, rect_2d_t * rect) {
	gfx_char_draw_inline(character, x, y, colour, rect);
}

inline void gfx_char_p_draw_inline(uint16_t character, int x, int y, uint32_t colour, rect_2d_t * rect) {
	uint32_t position = (y * rect->size.width) + x;
	uint32_t chr = font_get_character(character); // cant call it char, GRAHHHH
	font_drawer_t drawer = font_get_drawer(chr);
	int line = y;
	int pixel = 0;
	for (int i = 0; i < 128; i++) {
		if (pixel == 8) {
			line++; // go to the next line
			position = (line * rect->size.width) + x;
			pixel = 0; // go back to the start of the line
		}
		if (position > (rect->size.height * rect->size.width)) {
			continue;
		}
		drawer(rect->fb, font[chr][i + 2], colour, position + pixel);
		pixel++;
	}
}

void gfx_char_p_draw(uint16_t character, int x, int y, uint32_t colour, rect_2d_t * rect) {
	gfx_char_p_draw_inline(character, x, y, colour, rect);
}

void gfx_scroll(rect_2d_t * rect) {
	memcpy32(rect->fb, rect->fb + (rect->size.width * 16), (rect->size.height - 16) * rect->size.width);
	memset32(rect->fb + ((rect->size.height - 16) * rect->size.width), background_colour, (rect->size.width * 16));
}

int gfx_string_p_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect) {
	int position = x;
	int yoffset = 0;
	int char_width = (int) (rect->size.width / 8); // height and width in chracters
	int char_height = (int) (rect->size.height / 16);
	for (int i = 0; i < ustrlen(string); i++) {
		if ((y / 16) + ((yoffset / 16) + 1) + (position / 8) / char_width > char_height) {
			yoffset -= 16;
		}
		if (string[i] != u'\n') {
			gfx_char_p_draw_inline(string[i], position, y + yoffset, colour, rect);
			if (font[font_get_character(string[i])][1] == FONT_COMBINING) {
					position -= 8;
			}
		}
		position += 8;
		if (string[i] == u'\n' || (position / 8) >= char_width) {
			position = 0;
			yoffset += 16;
		}
	}
}

int gfx_string_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect) {
	int position = x;
	int yoffset = 0;
	int char_width = (int) (rect->size.width / 8); // height and width in chracters
	int char_height = (int) (rect->size.height / 16);
	for (int i = 0; i < ustrlen(string); i++) {
		if (y + (yoffset + 1) + position / char_width > char_height) {
			gfx_scroll(rect);
			yoffset--;
		}
		if (string[i] != u'\n') {
			gfx_char_draw_inline(string[i], position, y + yoffset, colour, rect);
			if (font[font_get_character(string[i])][1] == FONT_COMBINING) {
					position--;
			}
		}
		position++;
		if (string[i] == u'\n' || position >= char_width) {
			position = 0;
			yoffset++;
		}
	}
	rect->cursor.x = position % char_width;
	rect->cursor.y = y + yoffset;
}

int txt_string_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect) {
	if (colour >= 16) {
		return gfx_string_draw(string, x, y, colour, rect);
	} else {
		return gfx_string_draw(string, x, y, legacy_colour[colour], rect);
	}
}

int txt_string_p_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect) {
	if (colour >= 16) {
		return gfx_string_p_draw(string, x, y, colour, rect);
	} else {
		return gfx_string_p_draw(string, x, y, legacy_colour[colour], rect);
	}
}

// from gdk-pixbuf (gdk-pixbuf/pixops/pixops.c), optimised even further and commented
inline uint32_t alpha_calculate(uint32_t top, uint32_t bottom) {
	uint32_t alpha = top >> ALPHA_SHIFT; // grab the alpha value
	uint32_t inverted = COLOUR_MASK - alpha; // invert it (80% of the top == 20% of the bottom)
	uint32_t top_r = (top >> RED_SHIFT) & COLOUR_MASK; // grab all the top's rgb values
	uint32_t top_g = (top >> GREEN_SHIFT) & COLOUR_MASK;
	uint32_t top_b = (top >> BLUE_SHIFT) & COLOUR_MASK;
	uint32_t bottom_r = (bottom >> RED_SHIFT) & COLOUR_MASK; // grab the bottom's rgb values
	uint32_t bottom_g = (bottom >> GREEN_SHIFT) & COLOUR_MASK;
	uint32_t bottom_b = (bottom >> BLUE_SHIFT) & COLOUR_MASK;
	uint32_t output = 0;
	uint32_t tmp = 0; // result

	// multiply each RGB component by the alpha value
	// the result is the same as regular alpha calculation but times 256
	// the result is also 1 off so we have to add 256 (1 * 256) to it
	tmp = (alpha * top_r) + (inverted * bottom_r) + 256;

	output |= (tmp << 8) & RED_MASK; // shift into place and mask out the garbage
	tmp = (alpha * top_g) + (inverted * bottom_g) + 256;
	output |= tmp & GREEN_MASK; // mask out the garbage
	tmp = (alpha * top_b) + (inverted * bottom_b) + 256;
	output |= tmp >> 8; // shift over to blue
	output |= ALPHA_MASK; // make it opaque
	return output;
}

void rect_2d_draw(rect_2d_t * rect2, rect_2d_t * rect) {
	uint32_t * dest;
	uint32_t * src;
	int height = rect->size.height; // width !
	int width = rect->size.width;
	int ry = rect->y; // position !
	int rx = rect->x;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;

	int r1premult_width = width * 4;
	int r2premult_width = rect2->size.width * 4;

	// clip image if its outside bounds
	if ((ry + height) >= rect2->size.height) {
		height -= (ry + height) - rect2->size.height;
	}
	if ((rx + width) >= rect2->size.width) {
		width -= (rx + width) - rect2->size.width;
	}
	for (int y = starty; y < height; y++) {
		src = ((void *) rect->fb) + (y * r1premult_width) + (startx * 4);
		dest = ((void *) rect2->fb) + ((y + ry) * r2premult_width) + ((startx + rx) * 4);
		for (int x = startx; x < width; x++) {
			*dest++ = *src++;
		}
	}
}

void rect_2d_adraw(rect_2d_t * rect2, rect_2d_t * rect) {
	uint32_t * dest;
	uint32_t * src;
	int height = rect->size.height; // width !
	int width = rect->size.width;
	int ry = rect->y; // position !
	int rx = rect->x;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;

	int r1premult_width = width * 4;
	int r2premult_width = rect2->size.width * 4;

	// clip image if its outside bounds
	if ((ry + height) >= rect2->size.height) {
		height -= (ry + height) - rect2->size.height;
	}
	if ((rx + width) >= rect2->size.width) {
		width -= (rx + width) - rect2->size.width;
	}
	for (int y = starty; y < height; y++) {
		src = ((void *) rect->fb) + (y * r1premult_width) + (startx * 4);
		dest = ((void *) rect2->fb) + ((y + ry) * r2premult_width) + ((startx + rx) * 4);
		for (int x = startx; x < width; x++) {
			uint32_t colour = *src++;
			uint32_t top;
			if ((colour & 0xff000000) == 0) {
				dest++;
				continue;
			}
			if ((colour & 0xff000000) == 0xff000000) {
				*dest++ = colour;
				continue;
			}
			top = *dest;
			*dest++ = alpha_calculate(colour, top);
		}
	}
	return;
}

// fast transparent draw
void rect_2d_afdraw(rect_2d_t * rect2, rect_2d_t * rect) {
	uint32_t * dest;
	uint32_t * src;
	int height = rect->size.height; // width !
	int width = rect->size.width;
	int ry = rect->y; // position !
	int rx = rect->x;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;

	int r1premult_width = width * 4;
	int r2premult_width = rect2->size.width * 4;

	// clip image if its outside bounds
	if ((ry + height) >= rect2->size.height) {
		height -= (ry + height) - rect2->size.height;
	}
	if ((rx + width) >= rect2->size.width) {
		width -= (rx + width) - rect2->size.width;
	}
	for (int y = starty; y < height; y++) {
		src = ((void *) rect->fb) + (y * r1premult_width) + (startx * 4);
		dest = ((void *) rect2->fb) + ((y + ry) * r2premult_width) + ((startx + rx) * 4);
		for (int x = startx; x < width; x++) {
			uint32_t colour = *src++;
			if ((colour & 0xff000000) == 0) {
				dest++;
				continue;
			}
			*dest++ = colour;
		}
	}
}

void gfx_ndraw_rect(rect_2d_t * rect2, int width, int height, int x, int y, uint32_t colour) {
	uint32_t * dest;
	uint32_t * src;
	int ry = y; // position !
	int rx = x;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;

	int r1premult_width = width * 4;
	int r2premult_width = rect2->size.width * 4;

	// clip image if its outside bounds
	if ((ry + height) >= rect2->size.height) {
		height -= (ry + height) - rect2->size.height;
	}
	if ((rx + width) >= rect2->size.width) {
		width -= (rx + width) - rect2->size.width;
	}
	for (int y = starty; y < height; y++) {
		dest = ((void *) rect2->fb) + ((y + ry) * r2premult_width) + ((startx + rx) * 4);
		memset32(dest, colour, width - startx);
	}
}

inline void gfx_fbdraw_rect(rect_2d_t * rect2, int width, int height, int x, int y, uint32_t * fb) {
	uint32_t * dest;
	uint32_t * src;
	int ry = y; // position !
	int rx = x;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;

	int r1premult_width = width * 4;
	int r2premult_width = rect2->size.width * 4;

	// clip image if its outside bounds
	if ((ry + height) >= rect2->size.height) {
		height -= (ry + height) - rect2->size.height;
	}
	if ((rx + width) >= rect2->size.width) {
		width -= (rx + width) - rect2->size.width;
	}
	for (int y = starty; y < height; y++) {
		src = ((void *) fb) + (y * r1premult_width) + (startx * 4);
		dest = ((void *) rect2->fb) + ((y + ry) * r2premult_width) + ((startx + rx) * 4);
		memcpy32(dest, src, width - startx);
	}
}

// this is broken in a subtle way
inline void rect_2d_memset32(rect_2d_t * rect, uint32_t colour, int length, int x, int y) {
	int startx = (x >= 0) ? abs32(x) : 0;
	int stopx = (x < 0) ? abs32(x) : 0;
	int width = rect->size.width;
	if (x >= width || y >= rect->size.height || y < 0) {
		return; // ofscreen (breakage happens if we let this continue, this is the less bad option)
	}
	if ((x + length) >= width) { // clip
		length -= (x + length) - width;
	}
	memset32(rect->fb + (y * width) + startx, colour, length - stopx);
}

uint8_t degrade_step(uint8_t c, uint8_t t) {
	if (c == t) {
		return c;
	}
	if (t > c) {
		return c + 1;
	}
	return c - 1;
}

uint32_t rgb_degrade(uint32_t colour, uint32_t target) {
	uint8_t r, g, b;
	uint8_t tr, tg, tb;
	r = colour >> 16;
	g = colour >> 8;
	b = colour;
	tr = target >> 16;
	tg = target >> 8;
	tb = target;
	colour |= 0xff000000;
	target |= 0xff000000;
	if (target == colour) {
		return target;
	}
	r = degrade_step(r, tr);
	g = degrade_step(g, tg);
	b = degrade_step(b, tb);
	return (r << 16) | (g << 8) | b | 0xff000000;
}

uint32_t draw_taskbar_button(taskbar_button_t * button, uint32_t x) {
	uint16_t * p = button->text;
	int i = 0;
	int len = *p == 0 ? 0 : ustrlen(p);
	int text_margin = *p == 0 ? 0 : TASKBAR_TEXT_MARGIN;
	rect_2d_t icon = {
		(TASKBAR_MARGIN * 2) + 1 + x + (i * TASKBAR_CHAR_SIZE), 8, {0, 0},
		button->icon,
		{16, 16}
	};

	rect_2d_adraw(&taskbar, &icon);

	while (i < len) {
		gfx_char_p_draw_inline(*p++, 28 + x + (i * TASKBAR_CHAR_SIZE), 8, 0, &taskbar);
		i++;
	}

	button->x = TASKBAR_MARGIN + x;
	button->y = (TASKBAR_MARGIN * 2) + 1 + TASKBAR_ICON_SIZE + (len * TASKBAR_CHAR_SIZE) + text_margin + x;
	memset32(taskbar.fb + (root_window.size.width * 2) + TASKBAR_MARGIN + x + (TASKBAR_Y_START * taskbar.size.width), button_border, TASKBAR_ICON_SIZE + (len * TASKBAR_CHAR_SIZE) + text_margin + (TASKBAR_MARGIN * 2) + 2);
	memset32(taskbar.fb + (root_window.size.width * 2) + TASKBAR_MARGIN + x + (TASKBAR_Y_END * taskbar.size.width), button_border, TASKBAR_ICON_SIZE + (len * TASKBAR_CHAR_SIZE) + text_margin + (TASKBAR_MARGIN * 2) + 2);
	for (int i = TASKBAR_Y_END - 1; i > TASKBAR_Y_START; i--) {
		taskbar.fb[(root_window.size.width * 2) + TASKBAR_MARGIN + x + (i * taskbar.size.width)] = button_border;
		taskbar.fb[(root_window.size.width * 2) + TASKBAR_MARGIN + x + (TASKBAR_MARGIN * 2) + 1 + TASKBAR_ICON_SIZE + (len * TASKBAR_CHAR_SIZE) + text_margin + (i * taskbar.size.width)] = button_border;
	}
	return TASKBAR_ICON_SIZE + (len * TASKBAR_CHAR_SIZE) + (TASKBAR_MARGIN * 2) + 1 + text_margin + (TASKBAR_MARGIN * 2);
}

int draw_taskbar_callback(linked_t * node, void * p) {
	taskbar_button_t * button = node->p;
	button_offset += draw_taskbar_button(button, button_offset);
}

void draw_taskbar_buttons() {
	if (!taskbar_updated) {
		return;
	}
	button_offset = 0;
	memset32(taskbar.fb + (root_window.size.width * 2), taskbar_colour, root_window.size.width * taskbar_height);
	linked_iterate(taskbar_buttons, draw_taskbar_callback, 0);
	taskbar_updated = 0;
}

static inline int gfx_clip_height(int length) {
	if (length > background.size.height) {
		return background.size.height;
	}
	return length;
}

// completely incomprehensible
static inline void gfx_draw_tertiary(window_t * window, int x, int y) {
	int length = 0;
	taskbar_button_t * taskbar_button = window->taskbar;
	int width = window->drawwidth;
	int height = window->rect.size.height; // window->rect.size.
	register int x_offset = x + 1 + WINDOW_PADDING; // precomputed X coordinate
	register rect_2d_t * buffer = &back_buffer;
	rect_2d_memset32(buffer, 0xffffffff, width + 2 + (WINDOW_PADDING * 2), x, y);
	rect_2d_memset32(buffer, window_background, width, x_offset, y + 1);
	rect_2d_memset32(buffer, window_background, width, x_offset, y + 2);
	rect_2d_memset32(buffer, window_background, width, x_offset, y + TERTIARY_HEIGHT + (TERTIARY_MARGIN * 2) + window->rect.size.height + 1);
	rect_2d_memset32(buffer, window_background, width, x_offset, y + TERTIARY_HEIGHT + (TERTIARY_MARGIN * 2) + window->rect.size.height + 2);
	rect_2d_memset32(buffer, 0xff000000, width + 2 + (WINDOW_PADDING * 2), x, y + TERTIARY_HEIGHT + (TERTIARY_MARGIN * 2) + window->rect.size.height + WINDOW_PADDING + 1);
	rect_2d_memset32(buffer, window_background, width, x_offset, y + TERTIARY_HEIGHT + TERTIARY_MARGIN + 1);
	rect_2d_memset32(buffer, window_background, width, x_offset, y + TERTIARY_HEIGHT + TERTIARY_MARGIN + 2);
	if (x >= 0) {
		length = gfx_clip_height(y + TERTIARY_HEIGHT + (TERTIARY_MARGIN * 2) + height + WINDOW_PADDING + 1);
		for (int i = y + 1; i < length; i++) {
			buffer->fb[(i * buffer->size.width) + x] = 0xffffffff;
		}
		for (int i = y + 1; i < length; i++) {
			buffer->fb[(i * buffer->size.width) + x + 1] = window_background;
			buffer->fb[(i * buffer->size.width) + x + 2] = window_background;
		}
	}

	x_offset = x + 1 + width + (WINDOW_PADDING * 2);
	if (x_offset < buffer->size.width && x >= 0) {
		for (int i = y + 1; i < length; i++) {
			buffer->fb[(i * buffer->size.width) + x_offset] = 0xff000000;
		}
	}
	x_offset = x + 2 + width + WINDOW_PADDING;
	if (x_offset < buffer->size.width && x >= 0) {
		for (int i = y + 1; i < length; i++) {
			buffer->fb[(i * buffer->size.width) + x_offset] = window_background;
		}
	}
	x_offset = x + 1 + width + WINDOW_PADDING;
	if (x_offset < buffer->size.width && x >= 0) {
		for (int i = y + 1; i < length; i++) {
			buffer->fb[(i * buffer->size.width) + x_offset] = window_background;
		}
	}

	rect_2d_t icon = {
		1, 1, {0, 0},
		window->taskbar->icon,
		{16, 16}
	};

	rect_2d_draw(&window->text_rect, &icon);
	rect_2d_draw(buffer, &window->text_rect);
}

// predraw the tertiary text
// this solves many problems
// - performance
// - text disapearing near the screen borders
// - the '...' dispearing near screen borders
void gfx_predraw_text(window_t * window) {
	uint16_t * p = window->text;
	int len = *p == 0 ? 0 : ustrlen(p);
	int i = 0;
	int text_clipped = 0; // was the text clipped
	int width = window->rect.size.width; // window size
	free(window->text_rect.fb);
	window->text_rect.fb = malloc((width * TERTIARY_HEIGHT) * 4);
	window->text_rect.size.height = TERTIARY_HEIGHT;
	window->text_rect.size.width = width;
	memset32(window->text_rect.fb, tertiary_colour, width * TERTIARY_HEIGHT);

	if (((len * 8) + 1 + TERTIARY_ICON_MARGIN + TASKBAR_ICON_SIZE + TERTIARY_TEXT_PADDING) > width) {
		len = ((width - 1 - TERTIARY_ICON_MARGIN - TASKBAR_ICON_SIZE - TERTIARY_TEXT_PADDING) / 8) - 3;
		text_clipped = 1;
	}

	while (i < len) {
		gfx_char_p_draw_inline(*p++, (1 + TASKBAR_ICON_SIZE + TERTIARY_ICON_MARGIN) + (i * 8), 1, 0xffffffff, &window->text_rect);
		i++;
	}

	if (!text_clipped) {
		return;
	}
	len += 3;
	while (i < len) {
		gfx_char_p_draw_inline(u'.', (1 + TASKBAR_ICON_SIZE + TERTIARY_ICON_MARGIN) + (i * 8), 1, 0xffffffff, &window->text_rect);
		i++;
	}
}

static inline void gfx_update_window(window_t * window) {
	if ((window->x != window->drawx) || (window->y != window->drawy)) {
		window->onmove(window->priv); // draw coordinates will contain the previous location
		window->drawx = window->x;
		window->drawy = window->y;
		window->rect.y = window->drawy + TERTIARY_HEIGHT + (TERTIARY_MARGIN * 2) + 1; // top and bottom margin
		window->rect.x = window->drawx + 1 + WINDOW_PADDING; // left padding
		window->text_rect.x = window->rect.x;
		window->text_rect.y = window->drawy + 1 + TERTIARY_MARGIN;
	}
	int width = window->rect.size.width; // window size
	if (width != window->drawwidth) {
		window->onresize(window->priv); // tell window whats happening
		window->textupdate(window->priv);
		gfx_predraw_text(window);
		window->drawwidth = width; // update draw size
	}
}

static inline void gfx_draw_window(window_t * window) {
	gfx_update_window(window);
	window->ondraw(window->priv);
	gfx_draw_tertiary(window, window->drawx, window->drawy);
	rect_2d_afdraw(&back_buffer, &window->rect);
}

// GET THIS SHIT OUT OF HEREEEE PUT THIS IN THE STANDARD LIBRARY
void gfx_create_messagebox(int type, uint16_t * title, uint16_t * text) {
	int length = ustrlen(text);
	int height = MBOX_ICON_SIZE + 24; // image height + padding
	int width = MBOX_ICON_SIZE + MBOX_GAP + (length * 8) + 24; // image width + text gap + text width + padding
	window_t * window = gfx_create_window(title, u"messagebox", width, height);
	memset32(window->rect.fb, window_background, width * height);
	memcpy32(window->taskbar->icon, lemonos_icon, 256);

	uint32_t * icon = error_big_icon;
	switch (type) {
		case MBOX_INFO:
			icon = info_big_icon;
			break;
	}

	int top_left = MBOX_PADDING;
	int text_x = top_left + MBOX_ICON_SIZE + MBOX_GAP; // next to icon (+ GAP)
	gfx_fbdraw_rect(&window->rect, MBOX_ICON_SIZE, MBOX_ICON_SIZE, top_left, top_left, icon);
	txt_string_p_draw(text, text_x, top_left, 0xff000000, &window->rect);

	process_t * process = *get_current_process();
	process->windows = linked_add(process->windows, window);
}


int gfx_draw_window_callback(linked_t * node, void * p) {
	window_t * window = node->p;
	gfx_draw_window(window);
}

inline void draw_windows() {
	linked_iterate(windows, gfx_draw_window_callback, 0);
}

inline void draw_cursor() {
	if (cursor_disabled) {
		return;
	}
	rect_2d_afdraw(&back_buffer, &cursor);
}

void vga_dump_palette() {
	outb(0x03c6, 0xff);
	outb(0x03c8, 0);
	if (root_window.bpp == 4) {
		for (int i = 0; i < 256; i++) {
			// treat index like 1:1:1 colour
			// - NOTE: we cant do 1:2:1 because it seems the attribute controller steals a bit from us, :(
			uint8_t r = ((i >> 2) & 0b1) * 0xff;
			uint8_t g = ((i >> 1) & 0b1) * 0xff;
			uint8_t b = (i & 0b1) * 0xff;
			outb(0x03c9, r); // throw these at it directly
			outb(0x03c9, g);
			outb(0x03c9, b);
		}
		return;
	}
	for (int i = 0; i < 256; i++) {
		uint8_t r = ((i >> 5) & 0b111) * 36; // treat the index like a 3:3:2 colour and convert it to 24 bit
		uint8_t g = ((i >> 2) & 0b111) * 36;
		uint8_t b = (i & 0b11) * 85;
		r /= 4; // VGA wants 6 bit colour
		g /= 4;
		b /= 4;
		outb(0x03c9, r); // throw that at it
		outb(0x03c9, g);
		outb(0x03c9, b);
	}
}

uint8_t rgb32torgb4(uint32_t colour) {
	uint8_t r = (colour >> 16) & 0xff; // grab each channel
	uint8_t g = (colour >> 8) & 0xff;
	uint8_t b = colour & 0xff;
	r /= 0x80; // make each channel 1 bit
	g /= 0x80;
	b /= 0x80;
	return (b << 3) | (g << 2) | (r << 1); // assemble into 1:1:1 BGR colour (shifted to left by 1 bit)
}

uint8_t rgb4toattr(uint8_t * pixels, int which) {
	uint8_t attr = 0;
	// grab `which` bit from each pixel in reverse order and 
	for (int i = 0; i < 8; i++) {
		attr |= ((pixels[7 - i] >> which) & 1) << i;
	}
	return attr;
}

/* The QEMU/Virtualbox/Bochs VGA adapters seems to require the width and height
   to be multiples of 8, this restriction may also exist on real hardware, so
*/
int rgb_align(int component, int bpp) {
	if (component < 16) {
		return 16; // my stupid chud math doesnt work below 16
	}
	return (component / 8) * 8;
}

void rect_2d_crunch(rect_2d_t * rect, uint32_t * fb) {
	switch (rect->bpp) {
		case 4: {
			uint8_t * output = (uint8_t *) rect->fb;
			int size = rect->size.width * rect->size.height;
			size /= 8;
			int i = 0;
			while (size--) {
				uint8_t pixels[8];
				pixels[0] = rgb32torgb4(*fb++); // convert next 8 pixels to 4 bit
				pixels[1] = rgb32torgb4(*fb++);
				pixels[2] = rgb32torgb4(*fb++);
				pixels[3] = rgb32torgb4(*fb++);
				pixels[4] = rgb32torgb4(*fb++);
				pixels[5] = rgb32torgb4(*fb++);
				pixels[6] = rgb32torgb4(*fb++);
				pixels[7] = rgb32torgb4(*fb++);

				*output++ = rgb4toattr(pixels, 3); // now stripe all 8 pixels across 32 bits
				*output++ = rgb4toattr(pixels, 2);
				*output++ = rgb4toattr(pixels, 1);
				*output++ = rgb4toattr(pixels, 0);
			}
			return;
		}

		case 8: {
			uint8_t * byte = (uint8_t *) rect->fb;
			int size = rect->size.width * rect->size.height;
			while (size--) {
				uint32_t colour = *fb++;
				uint8_t r = (colour >> 16) & 0xff;
				uint8_t g = (colour >> 8) & 0xff;
				uint8_t b = colour & 0xff;
				r /= 32; // 32
				g /= 32;
				b /= 64; // 67
				*byte++ = (r << 5) | (g << 2) | b;
			}
			return;
		}

		case 15: {
			uint16_t * word = (uint16_t *) rect->fb;
			int size = rect->size.width * rect->size.height;
			while (size--) {
				uint32_t colour = *fb++;
				uint8_t r = (colour >> 16) & 0xff;
				uint8_t g = (colour >> 8) & 0xff;
				uint8_t b = (colour >> 0) & 0xff;
				r /= 8;
				g /= 8;
				b /= 8;
				*word++ = ((r << 10) | (g << 5) | b) & 0xffff;
			}
			return;
		}

		case 16: {
			uint16_t * word = (uint16_t *) rect->fb;
			int size = rect->size.width * rect->size.height;
			while (size--) {
				uint32_t colour = *fb++;
				uint8_t r = (colour >> 16) & 0xff;
				uint8_t g = (colour >> 8) & 0xff;
				uint8_t b = (colour >> 0) & 0xff;
				r /= 8;
				g /= 4;
				b /= 8;
				*word++ = ((r << 11) | (g << 5) | b) & 0xffff;
			}
			return;
		}

		case 24: {
			uint16_t * word = (uint16_t *) rect->fb;
			uint8_t * byte = ((uint8_t *) rect->fb) + 2;
			int size = rect->size.width * rect->size.height;
			while (size--) {
				uint32_t colour = *fb++;
				*word = colour & 0xffff;
				*byte = (colour >> 16) & 0xff;
				word = (uint16_t *) (((uint32_t) word) + 3);
				byte = (uint8_t *) (((uint32_t) byte) + 3);
			}
			return;
		}

		case 32: {
			int size = rect->size.width * rect->size.height;
			memcpy32(rect->fb, fb, size);
			return;
		}
	}
}

void software_draw_frame() {
	draw_taskbar_buttons();
	memcpy32(back_buffer.fb, background.fb, background_size);
	draw_windows();
	memcpy32(((void *) back_buffer.fb) + taskbar_y_offset, taskbar.fb, taskbar_size);
	draw_cursor();
	if (!gfx_one_fb) {
		if (root_window.bpp != 32) {
			rect_2d_crunch(&root_window, back_buffer.fb);
			return;
		}
		memcpy32(root_window.fb, back_buffer.fb, back_buffer_size);
	}
}

void draw_frame() {
	if (gfx_init_done == 0) {
		return;
	}
	software_draw_frame();
}

void disable_cursor_icon() {
	cursor_disabled = 1;
}

void enable_cursor_icon() {
	cursor_disabled = 0;
}

// todo: impliment
int gfx_startbutton_clicked(void * priv) {
	char * argv[] = {
		"terminal", "-h", "180"
	};
	int pid = exec_initrd("terminal", 1, argv);
	if (pid == 0) {
		return 0;
	}
	process_t * process = multitasking_get_pid(pid);
	process->killed = 0;
	return 0;
}

int gfx_search_callback(linked_t * node, void * p) {
	if (((uintptr_t) node->p) == ((uintptr_t) p)) {
		return 1;
	}
	return 0;
}

linked_t * gfx_find_taskbar(taskbar_button_t * button) {
	return linked_find(taskbar_buttons, gfx_search_callback, button);
}

linked_t * gfx_find_window2(window_t * window) {
	return linked_find(windows, gfx_search_callback, window);
}

int gfx_default_callback(void * priv) {
	return 0;
}

void gfx_default_event_handler(event_t * event, void * priv) {}

taskbar_button_t * gfx_create_taskbar(uint16_t * text, gfx_callback_t onclick) {
	taskbar_button_t * button = malloc(sizeof(taskbar_button_t) + 8);
	button->text = ustrdup(text);
	button->icon = malloc(1024);
	button->click = onclick;
	button->contextmenu = gfx_default_callback;
	button->textupdate = gfx_default_callback;
	taskbar_buttons = linked_add(taskbar_buttons, button);
	taskbar_button_count++;
	taskbar_updated = 1;
	return button;
}

void gfx_move_window(window_t * window, int x, int y) {
	int width = root_window.size.width;
	int height = root_window.size.height - taskbar.size.height;
	int oldx = window->x;
	int oldy = window->y;

	int bottom = (WINDOW_PADDING * 2) - TERTIARY_HEIGHT;
	int newx = x; // (x < -width + 8) ? 0 : (x > width - 8) ? width - 8 : x;
	int newy = (y < 0) ? 0 : (y > height - bottom) ? height - bottom : y;
	if (oldx == newx && oldy == newy) {
		return;
	}
	window->x = newx; // store in window
	window->y = newy;
}

window_t * gfx_create_window(uint16_t * text, uint16_t * progname, int width, int height) {
	window_t * window = malloc(sizeof(window_t) + 8);
	taskbar_button_t * taskbar = gfx_create_taskbar(progname, gfx_default_callback);
	memcpy(taskbar->icon, error_icon, 1024);
	memset(window, 0, sizeof(window_t));
	window->rect.cursor.x = 0;
	window->rect.cursor.y = 0;
	window->x = (root_window.size.width / 2) - (width / 2);
	window->y = (root_window.size.height / 2) - (height / 2);
	window->rect.size.width = width;
	window->rect.size.height = height;
	window->rect.fb = malloc((width * height) * 4);
	window->text = ustrdup(text);
	window->id = window_count;
	window->shown = 1;
	window->onopen = gfx_default_callback;
	window->onfocus = gfx_default_callback;
	window->onclose = gfx_default_callback;
	window->ondraw = gfx_default_callback;
	window->onopen = gfx_default_callback;
	window->onresize = gfx_default_callback;
	window->onmove = gfx_default_callback;
	window->textupdate = gfx_default_callback;
	window->send_event = gfx_default_event_handler;
	window->taskbar = taskbar;
	windows = linked_add(windows, window);
	window_count++;
	return window;
}

void gfx_set_title(window_t * window, uint16_t * text) {
	free(window->text);
	window->text = ustrdup(text);
}

void gfx_taskbar_set_text(taskbar_button_t * button, uint16_t * text) {
	free(button->text);
	button->text = ustrdup(text);
	taskbar_updated = 1;
}

void gfx_set_progname(window_t * window, uint16_t * text) {
	gfx_taskbar_set_text(window->taskbar, text);
}

void gfx_cleanup_window(window_t * window) {
	taskbar_button_t * button;
	button = window->taskbar;
	free(button->text);
	free(button->icon);
	free(button);
	taskbar_buttons = linked_delete(gfx_find_taskbar(window->taskbar));
	windows = linked_delete(gfx_find_window2(window));
	free(window->text);
	free(window->rect.fb);
	free(window->text_rect.fb);
	free(window);
	taskbar_updated = 1;
}

void gfx_close_window(window_t * window) {
	disable_interrupts();
	linked_t * node = linked_find(windows, gfx_get_window, window);
	if (!node) {
		return;
	}
	window_count--;
	windows = linked_delete(node);
	gfx_cleanup_window(window);
	enable_interrupts();
}

int gfx_find_button(linked_t * node, void * pass) {
	mouse_event_t * event = pass;
	taskbar_button_t * button = node->p;
	return (event->x >= button->x) && (event->x <= button->y);
}

int gfx_find_window(linked_t * node, void * pass) {
	mouse_event_t * event = pass;
	window_t * window = node->p;
	int width = window->rect.size.width + (WINDOW_PADDING * 2);
	int height = window->rect.size.height + TERTIARY_HEIGHT + (TERTIARY_MARGIN * 2) + WINDOW_PADDING;
	return (event->y > window->y) && (event->y < (window->y + height)) && (event->x > window->x) && (event->x < (window->x + width));
}

int gfx_get_window(linked_t * node, void * pass) {
	if (pass == node->p) {
		return 1;
	}
	return 0;
}

window_t * gfx_mouse_touching(mouse_event_t * event) {
	// windows are ordered by Z axis, so use find_back
	linked_t * node = linked_find_back(windows, gfx_find_window, event);
	if (!node) {
		return NULL;
	}
	return (window_t *) node->p;
}

void gfx_taskbar_event(mouse_event_t * mouseevent) {
	linked_t * node;
	taskbar_button_t * button;
	mouse_held_t * held = mouseevent->held;
	static int click = 0;
	if (held->left == click || held->left == 0) {
		click = held->left;
		return;
	}
	click = held->left;
	if (mouseevent->y < (taskbar.y + 3) || mouseevent->x < 3 || mouseevent->y > root_window.size.height - 3) {
		return;
	}
	node = linked_find(taskbar_buttons, gfx_find_button, mouseevent);
	if (!node) {
		return;
	}
	button = node->p;
	button->click(button->priv);
}

void gfx_event_window_tertiary(mouse_event_t * mouseevent, window_t * window) {
	mouse_held_t * held = mouseevent->held;
	static int click = 0;
	static int holding = 0;
	if (held->left != click) {
		click = held->left;
		holding = ((mouseevent->y > window->y + 1) && (mouseevent->x > window->x + 1) && (mouseevent->y < window->y + 3 + TERTIARY_HEIGHT) && (mouseevent->x < window->x + (WINDOW_PADDING * 2) + window->rect.size.width));
	}
	if (!window || !holding) {
		return;
	}
	gfx_move_window(window, window->x + mouseevent->bdelta_x, window->y - mouseevent->bdelta_y);
}

void gfx_send_window_event_mouse(mouse_event_t * event, window_t * window) {
	mouse_event_t * copy;
	int height = TERTIARY_HEIGHT + WINDOW_PADDING + TERTIARY_MARGIN; // tertiary hight plus margin
	int ax = event->x - window->x - WINDOW_PADDING; // adjusted x and y
	int ay = event->y - window->y - height;
	if ((ax < 0) || (ay < 0) || (ax > window->rect.size.width) || (ay > window->rect.size.height)) {
		return; // outside of window context
	}
	copy = malloc(sizeof(mouse_event_t) + 8);
	memcpy(copy, event, sizeof(mouse_event_t));
	copy->x = ax;
	copy->y = ay; // obbvious
	copy->bdelta_x = window->x - copy->x;
	copy->bdelta_y = window->y - height - event->y;
	window->send_event((event_t *) copy, window->priv);
	free(copy);
}

void gfx_send_window_event(event_t * event, window_t * window) {
	if (event->type == EVENT_MOUSE) {
		gfx_send_window_event_mouse((mouse_event_t *) event, window);
		return;
	}
	if (event->type == EVENT_JOYSTICK) {
		joystick_event_t * copy = malloc(sizeof(joystick_event_t) + 8);
		memcpy(copy, event, sizeof(joystick_event_t));
		window->send_event((event_t *) copy, window->priv);
		free(copy);
		return;
	}
	if (event->type != EVENT_KEYBOARD) {
		return;
	}
	kbd_event_t * keyevent = (kbd_event_t *) event;
	kbd_event_t * copy = malloc(sizeof(kbd_event_t) + 8);
	copy->type = EVENT_KEYBOARD;
	copy->pressed = keyevent->pressed;
	copy->keycode = keyevent->keycode;
	copy->held = keyevent->held;
	window->send_event((event_t *) copy, window->priv);
	free(copy);
}

void gfx_window_event(mouse_event_t * mouseevent) {
	linked_t * node;
	mouse_held_t * held = mouseevent->held;
	static window_t * window = NULL;
	static int holding = 0;
	static int click = 0;
	if (held->left != click) {
		click = held->left;
		if (click == 0 && holding) {
			window = NULL;
			holding = 0;
			return;
		}
		node = linked_find_back(windows, gfx_find_window, mouseevent);
		if (!node) {
			active_window = &fake_root_window;
			return;
		}
		window = node->p;
		if (window_count && window_count != 1) {
			windows = linked_pop(node);
			windows = linked_append(windows, node);
		}
		active_window = window;
		holding = mouseevent->y < window->y + 3 + TERTIARY_HEIGHT;
	}
	click = held->left;
	if (holding) {
		gfx_event_window_tertiary(mouseevent, window);
		return;
	}
}

void gfx_handle_ipc(event_t * event) {
	ipc_event_t * ipc = (ipc_event_t *) event;
	if (event->type != EVENT_IPC) {
		return;
	}

	window_spec_t * spec;
	process_t * from = ipc->from;
	linked_t * node;

	switch (ipc->cmd) {
		case GFX_IPC_CREATE_WINDOW:
			spec = ipc->data;
			spec->output = gfx_create_window(spec->title, spec->taskbar_text, spec->width, spec->height);
			from->windows = linked_add(from->windows, spec->output);
			return;
	}
}

void gfx_handle_event(event_t * event) {
	if (event->type == EVENT_IPC) {
		return; // ignore other processes trying to talk to us, fuck off !
	}
	if (event->type == EVENT_MOUSE) {
		// drag windows and stuff
		if (mouse_y > taskbar.y) {
			gfx_taskbar_event((mouse_event_t *) event);
		} else {
			gfx_window_event((mouse_event_t *) event);
		}
	}
	// send event to active window
	gfx_send_window_event(event, active_window);
}

int gfx_videod_main() {
	int frame_divisor = pit_freq / 60;
	uint64_t frame_target = ticks + frame_divisor;
	uint64_t fps_target = ticks + pit_freq;
	int fps_tmp = 0;
	while (1) {
		if (ticks >= fps_target) {
			fps_target = ticks + pit_freq;
			fps = fps_tmp;
			fps_tmp = 0;
		}
		if (ticks >= frame_target || !locked_fps) {
			frame_target = ticks + frame_divisor - 1;
			fps_tmp++;
			draw_frame();
			yield();
		}
		if (resize_after_frame) {
			gfx_do_resize(resize_width, resize_height);
		}
	}
}

void gfx_do_resize(int width, int height) {
	int interrupts = interrupts_enabled();
	disable_interrupts();
	free(back_buffer.fb);
	free(taskbar.fb);

	root_window.size.width = width;
	root_window.size.height = height;

	back_buffer.fb = malloc((width * height) * 4);
	back_buffer.size.width = width;
	back_buffer.size.height = height;
	back_buffer_size = width * height;

	rect_2d_t rect = (rect_2d_t) {0, 0, (vect_2d_t) {0, 0}, malloc((width * (height - taskbar_height - 2)) * 4), (size_2d_t) {width, height - taskbar_height - 2}, 0, 0, 0};
	memset32(rect.fb, background_colour, rect.size.width * rect.size.height);
	rect_2d_draw(&rect, &background);
	free(background.fb);
	background.fb = rect.fb;
	background.size.width = rect.size.width;
	background.size.height = rect.size.height;
	background_size = rect.size.width * rect.size.height;

	taskbar.fb = malloc((width * (taskbar_height + 2)) * 4);
	taskbar.y = (height - taskbar_height) - 2;
	taskbar.size.width = width;
	taskbar.size.height = taskbar_height + 2;
	taskbar_y_offset = (taskbar.y * width) * 4;
	taskbar_size = taskbar.size.width * taskbar.size.height;
	memset32(taskbar.fb, taskbar_highlight_colour, width * 2);

	taskbar_updated = 1;
	interrupts_restore(interrupts);
}

void gfx_resize(int width, int height) {
	resize_width = width;
	resize_height = height;
	resize_after_frame = 1;
}

void gfx_resize_callback(display_t * display, int event, void * priv) {
	switch (event) {
		case DISPLAY_RESIZE:
			gfx_resize(display->width, display->height);
			return;
		case DISPLAY_CRUNCH:
			root_window.bpp = display->bpp;
			vga_dump_palette();
			return;
	}
}

void gfx_init() {
	// assert(multiboot_header->framebuffer_type == 0, GRAPHICS_ERROR, 0);
	// if the pointer is 64 bit then die
	assert(multiboot_header->framebuffer < 0xffffffff, RECIEVED_64BIT_POINTER, 0);

	root_window.fb = (uint32_t *) (uint32_t) multiboot_header->framebuffer;
	root_window.size.width = multiboot_header->framebuffer_width;
	root_window.size.height = multiboot_header->framebuffer_height;
	root_window.bpp = multiboot_header->framebuffer_bpp;
	//memset32(root_window.fb, 0x00000000, root_window.size.width * root_window.size.height);

	vga_dump_palette();

	back_buffer.fb = root_window.fb;
	if (!gfx_one_fb) {
		back_buffer.fb = malloc((root_window.size.width * root_window.size.height) * 4);
	}
	back_buffer.size.width = root_window.size.width;
	back_buffer.size.height = root_window.size.height;
	//memset32(back_buffer.fb, 0x00000000, root_window.size.width * root_window.size.height);
	back_buffer_size = root_window.size.width * root_window.size.height;

	background.fb = malloc((root_window.size.width * (root_window.size.height - taskbar_height - 2)) * 4);
	background.cursor.x = 0;
	background.cursor.y = 0;
	background.x = 0;
	background.y = 0;
	background.size.width = root_window.size.width;
	background.size.height = root_window.size.height - taskbar_height - 2;
	background.locked = 0;
	memset32(background.fb, background_colour, background.size.width * background.size.height);
	background_size = background.size.width * background.size.height;

	cursor.fb = malloc(1024);
	cursor.x = mouse_x;
	cursor.y = mouse_y;
	cursor.size.width = 16;
	cursor.size.height = 16;
	//memset(cursor.fb, 0, 1024);
	gfx_char_draw(u'\ue01e', 0, 0, 0xff000000, &cursor);

	taskbar.fb = malloc((root_window.size.width * (taskbar_height + 2)) * 4);
	taskbar.cursor.x = 0;
	taskbar.cursor.y = 0;
	taskbar.x = 0;
	taskbar.y = (root_window.size.height - taskbar_height) - 2;
	taskbar.size.width = root_window.size.width;
	taskbar.size.height = taskbar_height + 2;
	taskbar_y_offset = (taskbar.y * back_buffer.size.width) * 4;
	taskbar_size = taskbar.size.width * taskbar.size.height;
	memset32(taskbar.fb, taskbar_highlight_colour, root_window.size.width * 2);

	startbutton = gfx_create_taskbar(u"Start", gfx_startbutton_clicked);
	startbutton->contextmenu = gfx_startbutton_clicked;
	memcpy(startbutton->icon, start_icon, 1024);

	fake_root_window = (window_t) {root_window, ustrdup(u"root"), 0, 0, 0, 0, 0, 1};
	fake_root_window.onopen = gfx_default_callback;
	fake_root_window.onfocus = gfx_default_callback;
	fake_root_window.onclose = gfx_default_callback;
	fake_root_window.ondraw = gfx_default_callback;
	fake_root_window.onopen = gfx_default_callback;
	fake_root_window.send_event = gfx_default_event_handler;
	fake_root_window.taskbar = NULL;
	active_window = &fake_root_window;

	taskbar_updated = 1;
	gfx_init_done = 1;

	//*((uint32_t *) 0x5f0) = (uint32_t) gfx_char_p_draw;
}

void gfx_late_init() {
	process_t * process = create_process(u"videod",  gfx_videod_main);
	process->system = 1;
	process->recv_global_event = gfx_handle_event;
	process->recv_event = gfx_handle_ipc;
}
