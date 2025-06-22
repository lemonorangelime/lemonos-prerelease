#pragma once

#include <bus/pci.h>
#include <linked.h>

typedef struct display display_t;

typedef void (* display_resize_t)(display_t * display, int width, int height);
typedef void (* display_crunch_t)(display_t * display, int bpp);
typedef void (* display_listen_t)(display_t * display, int event, void * priv);

typedef struct {
	display_listen_t trigger;
	void * priv;
} display_listener_t;

typedef struct display {
	int type;
	pci_t * pci;
	display_resize_t resize;
	display_crunch_t crunch;
	int width;
	int height;
	int bpp;
	int selectable;
	void * fb;
	void * priv;
	linked_t * listeners;
} display_t;

enum {
	DISPLAY_VIRTUAL = 0,
	DISPLAY_VGA,
	DISPLAY_ACCELERATED,
};

enum {
	DISPLAY_DESTROY,
	DISPLAY_RESIZE,
	DISPLAY_CRUNCH,
	DISPLAY_RELOCATE,
};

display_t * display_create(int type, display_resize_t resize, display_crunch_t crunch);
void display_register(display_t * display);
void display_listen(display_t * display, display_listen_t callback, void * priv);
void display_trigger_listeners(display_t * display, int event);
void display_resize(display_t * display, int width, int height);
void display_crunch(display_t * display, int bpp);
void display_free(display_t * display);
display_t * display_get_default();
