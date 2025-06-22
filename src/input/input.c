#include <input/input.h>
#include <multitasking.h>
#include <graphics/graphics.h>
#include <linked.h>
#include <input/layout.h>
#include <stdio.h>
#include <memory.h>
#include <util.h>

linked_t * events = NULL; // queued event's list
static process_t * dispatchd; // dispatchd's process control block

// convert an event to a character using the current layout
// also process meta keys and keyboard layers
uint16_t event_to_char(kbd_event_t * event, int force_layer) {
	int layer = event->held->meta + event->held->metalock;
	int shift = event->held->lshift || event->held->rshift;
	uint16_t (* layout)[][98];
	if (force_layer != -1) {
		layer = force_layer;
	}
	layout = layout_get_layer(layer);
	return (*layout)[shift][event->keycode];
}

// convert a keyboard event to an ascii char (using US QWERTY keyboard)
//
// obviously non-meta or layer aware due to the whole US QWERTY thing, this could cause (minor)
// bugs in software that is not aware of this, as an example, sysrqd uses `event_to_ascii_char()`
// for it's event procesing:
//
//  uint16_t chr = event_to_ascii_char(keyevent);
//
// this means that `SUPER + SYSRQ + π` on a will list PCI devices, but so will `SUPER + SYSRQ + ϡ`
// in syrqd's case this is fine and intentional but it is something to be aware of
uint16_t event_to_ascii_char(kbd_event_t * event) {
	int shift = event->held->lshift || event->held->rshift;
	return us_qwerty_kbd[shift][event->keycode];
}

int broadcast_event_callback(linked_t * node, void * p) {
	process_t * process = node->p;
	event_t * event = p;
	if ((uint32_t) process->recv_global_event == 0) {
		return 0; // if they dont have an event handler just leave
	}
	// temporarily switch active process
	// this means that if for example you press SUPER + SYSRQ + H, the process state will be
	// | active_process == sysrqd
	// | current_process == dispatchd
	// this whole active process shenanigans may be ripped out of the multitasking API soon, but for now do this
	process_t * me = *get_active_process();
	*get_active_process() = process;
	process->recv_global_event(event);
	*get_active_process() = me;
}

// send event to everyone
void broadcast_event(event_t * event) {
	events = linked_add(events, event);
	dispatchd->killed = 0; // wake dispatchd if it was sleeping
}

// send event directly to process (ocassionally useful in IPC and graphics)
void send_event(event_t * event, process_t * process) {
	if ((uint32_t) process->recv_event == 0) {
		return; // if they dont have an event handler just leave
	}
	// temporarily switch active process, see the comment in broadcast_event about this
	process_t * me = *get_active_process();
	*get_active_process() = process;
	process->recv_event(event);
	*get_active_process() = me;
}

void eventd_dispatch() {
	while (1) {
		linked_t * node = 0;
		event_t * event = 0;
		// we could probably do this non-blocking, but this might unintuitively be more performant
		disable_interrupts();
		events = linked_shift(events, &node);
		if (!node) {
			dispatchd->killed = 1; // suicide
			enable_interrupts();
			continue;
		}
		enable_interrupts();
		event = node->p; // grab the event
		linked_iterate(procs, broadcast_event_callback, event); // now broadcast this
		free(event);
		free(node);
	}
}

void event_init() {
	process_t * proc = create_process(u"dispatchd", eventd_dispatch);
	proc->system = 1;
	proc->kill = force_alive_kill_handler;
	dispatchd = proc;
}

// debugging
void dump_event(event_t * event) {
	if (!gfx_init_done) {
		return;
	}
	printf(u"Event:\n");
	printf(u" - Type: %d\n", event->type);
	switch (event->type) {
		default:
			printf(u" - Unknown event type\n");
			return;
		case EVENT_KEYBOARD:
			kbd_event_t * keyevent = (kbd_event_t *) event;
			printf(u" - Keycode: 0x%x\n", keyevent->keycode);
			printf(u" - Pressed: %d\n", keyevent->pressed);
			printf(u" - Held:\n");
			printf(u"    - sysrq: %d\n", keyevent->held->sysrq);
			printf(u"    - lctrl: %d\n", keyevent->held->lctrl);
			printf(u"    - rctrl: %d\n", keyevent->held->rctrl);
			printf(u"    - super: %d\n", keyevent->held->super);
			printf(u"    - meta: %d\n", keyevent->held->meta);
			printf(u"    - lalt: %d\n", keyevent->held->lalt);
			printf(u"    - ralt: %d\n", keyevent->held->ralt);
			printf(u"    - lshift: %d\n", keyevent->held->lshift);
			printf(u"    - rshift: %d\n", keyevent->held->rshift);
			printf(u"    - caps: %d\n", keyevent->held->caps);
			printf(u"    - metalock: %d\n", keyevent->held->metalock);
			return;
	}
}
