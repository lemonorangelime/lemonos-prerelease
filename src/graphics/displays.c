#include <linked.h>
#include <memory.h>
#include <string.h>
#include <graphics/displays.h>

static linked_t * displays = NULL;

static display_t * display_alloc() {
	display_t * display = malloc(sizeof(display_t));
	memset(display, 0, sizeof(display_t));
	display->type = DISPLAY_VGA;
	return display;
}

display_t * display_create(int type, display_resize_t resize, display_crunch_t crunch) {
	display_t * display = display_alloc();
	display->type = type;
	display->resize = resize;
	display->crunch = crunch;
	if (type != DISPLAY_VIRTUAL) {
		display->selectable = (displays == NULL);
	}
	return display;
}

void display_register(display_t * display) {
	if (!display) {
		return;
	}
	displays = linked_add(displays, display);
}

void display_listen(display_t * display, display_listen_t callback, void * priv) {
	if (!display) {
		return;
	}
	display_listener_t * listener = malloc(sizeof(display_listener_t));
	listener->trigger = callback;
	listener->priv = priv;
	display->listeners = linked_add(display->listeners, listener);
}

void display_trigger_listeners(display_t * display, int event) {
	if (!display) {
		return;
	}
	linked_iterator_t iterator = {display->listeners};
	linked_t * node = linked_step_iterator(&iterator);
	while (node) {
		display_listener_t * listener = node->p;
		listener->trigger(display, event, listener->priv);
		node = linked_step_iterator(&iterator);
	}
}

display_t * display_get_default() {
	linked_iterator_t iterator = {displays};
	linked_t * node = linked_step_iterator(&iterator);
	while (node) {
		display_t * display = node->p;
		if (display->selectable) {
			return display;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

void display_resize(display_t * display, int width, int height) {
	if (!display) {
		return;
	}
	display->resize(display, width, height);
	display_trigger_listeners(display, DISPLAY_RESIZE);
}

void display_crunch(display_t * display, int bpp) {
	if (!display) {
		return;
	}
	display->crunch(display, bpp);
	display_trigger_listeners(display, DISPLAY_CRUNCH);
	display_trigger_listeners(display, DISPLAY_RESIZE); // sometimes a crunch can resize the display aswell, to be safe always trigger a resize event
}

void display_free(display_t * display) {
	if (!display) {
		return;
	}
	display_trigger_listeners(display, DISPLAY_DESTROY);
	linked_destroy_all(display->listeners, NULL, NULL);
	free(display);
}
