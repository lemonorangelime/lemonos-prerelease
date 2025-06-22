#pragma once

#include <graphics/displays.h>
#include <stdint.h>
#include <stdio.h>
#include <linked.h>
#include <math.h>
#include <input/input.h>

enum CHARACTER_TYPES {
	FONT_LEGACY,
	FONT_BLANK,
	FONT_COMBINING,
	FONT_TRUECOLOUR = 16,
};

extern int legacy_colour[16];

typedef void (* font_drawer_t)(uint32_t * fb, uint32_t chr, uint32_t colour, uint32_t position);
typedef void (* gfx_event_callback_t)(event_t * event, void * priv);
typedef int (* gfx_callback_t)(void * priv);
typedef void (* gfx_accelerator_call_t)();

typedef struct size_2d {
	int width;
	int height;
} size_2d_t;

typedef struct _rect_2d {
	int x;
	int y;
	vect_2d_t cursor;
	uint32_t * fb;
	size_2d_t size;
	int bpp;
	int updated;
	int locked;
} rect_2d_t;

typedef struct {
	uint32_t * icon;
	uint16_t * text;
	gfx_callback_t click;
	gfx_callback_t contextmenu;
	gfx_callback_t textupdate;
	void * priv;
	// misnomer
	int x;
	int y;
} taskbar_button_t;

typedef struct _window {
	rect_2d_t rect;
	uint16_t * text;
	rect_2d_t text_rect;
	int id;
	int x;
	int y;
	int drawx; // solve racist condition
	int drawy;
	int drawwidth;
	int padding;
	int shown; // hidden?
	int drawer;
	gfx_callback_t onopen;
	gfx_callback_t onfocus;
	gfx_callback_t onclose;
	gfx_callback_t ondraw;
	gfx_callback_t onresize;
	gfx_callback_t onmove;
	gfx_callback_t textupdate;
	void * priv;
	gfx_event_callback_t send_event;
	taskbar_button_t * taskbar;
} window_t;

typedef struct {
	uint16_t * title;
	uint16_t * taskbar_text;
	int width;
	int height;
	window_t * output;
} window_spec_t;

typedef struct {
	void * fb;
	int width;
	int height;
	int bpp;
} framebuffer_spec_t;

enum {
	BPP_32BIT,
	BPP_16BIT,
	BPP_8BIT,
};

enum {
	WINDOW_DRAWER_DRAW, // normal draw (no transparency)
	WINDOW_DRAWER_AFDRAW, // fast transparency (1 bit)
	WINDOW_DRAWER_ADRAW, // full alpha transparency
	WINDOW_DRAWER_CKDRAW, // chroma keyed
};

enum {
	LEGACY_COLOUR_BLACK,
	LEGACY_COLOUR_BLUE,
	LEGACY_COLOUR_GREEN,
	LEGACY_COLOUR_CYAN,
	LEGACY_COLOUR_RED,
	LEGACY_COLOUR_PURPLE,
	LEGACY_COLOUR_BROWN,
	LEGACY_COLOUR_GRAY,
	LEGACY_COLOUR_DARK_GREY,
	LEGACY_COLOUR_LIGHT_BLUE,
	LEGACY_COLOUR_LIGHT_GREEN,
	LEGACY_COLOUR_LIGHT_CYAN,
	LEGACY_COLOUR_LIGHT_RED,
	LEGACY_COLOUR_LIGHT_PURPLE,
	LEGACY_COLOUR_YELLOW,
	LEGACY_COLOUR_WHITE,
};

enum {
	GFX_IPC_CREATE_WINDOW,
};

#define COLOUR_MASK 0xff

#define RED_MASK 0xff0000
#define GREEN_MASK 0x00ff00
#define BLUE_MASK 0x0000ff
#define ALPHA_MASK 0xff000000

// half life reference
#define BLUE_SHIFT 0
#define GREEN_SHIFT 8
#define RED_SHIFT 16
#define ALPHA_SHIFT 24

extern rect_2d_t root_window;
extern rect_2d_t back_buffer;
extern rect_2d_t background;
extern rect_2d_t taskbar;
extern rect_2d_t cursor;
extern window_t * active_window;
extern linked_t * taskbar_buttons;
extern uint32_t button_offset;
extern int taskbar_height;
extern int fps;
extern int gfx_init_done;
extern int locked_fps;
extern int gfx_one_fb;
extern int taskbar_y_offset;
extern int taskbar_size;

extern uint32_t window_background;
extern uint32_t background_colour;

void rect_2d_draw(rect_2d_t * rect2, rect_2d_t * rect);
void rect_2d_adraw(rect_2d_t * rect2, rect_2d_t * rect);
void rect_2d_crunch(rect_2d_t * rect, uint32_t * fb);

void disable_cursor_icon();
void enable_cursor_icon();
int rgb_align(int component, int bpp);

void gfx_do_resize(int width, int height);
void gfx_resize(int width, int height);
void gfx_resize_callback(display_t * display, int event, void * priv);
void gfx_init();
void gfx_late_init();
void gfx_blank_window(window_t * window);
void gfx_blank_drawwindow(window_t * window);
void gfx_char_draw(uint16_t character, int x, int y, uint32_t colour, rect_2d_t * rect);
int gfx_string_p_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect);
int gfx_string_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect);
int txt_string_draw(uint16_t * string, int x, int y, uint32_t colour, rect_2d_t * rect);
uint32_t rgb_degrade(uint32_t colour, uint32_t target);

void gfx_cleanup_window(window_t * window);
void gfx_close_window(window_t * window);

int gfx_get_window(linked_t * node, void * pass);

void gfx_set_title(window_t * window, uint16_t * text);
void gfx_taskbar_set_text(taskbar_button_t * button, uint16_t * text);
void gfx_set_progname(window_t * window, uint16_t * text);

uint32_t draw_taskbar_button(taskbar_button_t * button, uint32_t x);

window_t * gfx_create_window(uint16_t * text, uint16_t *, int width, int height);
int open_window(window_t * window);
int close_window(window_t * window);
void draw_frame();

#define TASKBAR_MARGIN 3

// icon is sqaure (16x16)
#define TASKBAR_ICON_SIZE 16
#define TASKBAR_CHAR_SIZE 8

// position of top and bottom of tasbar button
#define TASKBAR_Y_START 3
#define TASKBAR_Y_END 24

// margin between icon and text
#define TASKBAR_TEXT_MARGIN 6

#define TERTIARY_HEIGHT 18
#define TERTIARY_MARGIN 2
#define TERTIARY_TEXT_PADDING 2

#define TERTIARY_ICON_MARGIN 4

#define WINDOW_PADDING 2


enum {
	MBOX_GAP = 10,
	MBOX_PADDING = 12,
	MBOX_ICON_SIZE = 32,
};

enum {
	MBOX_ERROR = 0,
	MBOX_INFO = 1,
};
