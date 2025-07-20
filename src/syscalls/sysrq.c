#include <input/input.h>
#include <multitasking.h>
#include <util.h>
#include <memory.h>
#include <pit.h>
#include <graphics/font.h>
#include <cpuid.h>
#include <interrupts/apic.h>
#include <stdio.h>
#include <string.h>
#include <drivers/input/keyboard.h>
#include <graphics/graphics.h>
#include <power/sleep.h>
#include <net/net.h>
#include <drivers/thermal/thermal.h>
#include <bin_formats/elf.h>
#include <bus/pci.h>
#include <bus/pcidevs.h>

// system request key

int sysrq_dump_process(linked_t * node, void * p) {
	process_t * process = node->p;
	cprintf(LEGACY_COLOUR_WHITE, u"PID %d : ", (int) process->pid);
	cprintf(LEGACY_COLOUR_WHITE, u"%s %s%s(%s)\n", process->name, process->system ? u"(System Process) " : u"", process->killed ? u"(Paused) " : u"", process->personality->name);
	cprintf(LEGACY_COLOUR_WHITE, u"STACK=%r EBP=%r ESP=%r EIP=%r\n", process->stack, process->ebp, process->esp, process->eip);
}

int sysrq_dump_interface(linked_t * node, void * p) {
	interface_t * interface = node->p;
	uint16_t * name = interface->drvname ? interface->drvname : u"null";
	cprintf(LEGACY_COLOUR_WHITE, u"%s(%d) %s%s: ", interface->name, interface->id, interface->main ? u"(default) " : u"", interface->working ? u"" : u" (Disabled)");
	cprintf(LEGACY_COLOUR_WHITE, u"%dMbps %s-duplex%s (Driver: %s)\n",
		interface->speed, interface->duplex ? u"full" : u"half", interface->autonegotiation ? u" (autonegotiation)" : u"", interface->drvname
	);
	cprintf(LEGACY_COLOUR_WHITE, u"Stats: %d sent (%d Bytes), %d received (%d bytes)\n",
		interface->stat_sent, interface->stat_byte_sent,
		interface->stat_recv, interface->stat_byte_recv
	);
}

int sysrq_dump_pci(linked_t * node, void * p) {
	pci_t * device = node->p;
	uint16_t * vendorname = pcidev_search_vendor(device->vendor);
	uint16_t * devicename = pcidev_search_device(device->vendor, device->device);
	printf(u"%d:%d.%d %w: %s %s [%w:%w]\n", device->bus, device->slot, device->function, device->class, vendorname, devicename, device->vendor, device->device);
}

int sysrq_dump_pci_deduped(linked_t * node, void * p) {
	pci_t * device = node->p;
	pci_t ** lastp = p;
	pci_t * last = *lastp;
	if (last && (last->bus == device->bus && last->slot == device->slot && last->class == device->class && last->vendor == device->vendor && last->device == device->device)) {
		return 0;
	}
	uint16_t * vendorname = pcidev_search_vendor(device->vendor);
	uint16_t * devicename = pcidev_search_device(device->vendor, device->device);
	printf(u"%d:%d.%d %w: %s %s [%w:%w]\n", device->bus, device->slot, device->function, device->class, vendorname, devicename, device->vendor, device->device);
	*lastp = device;
}

void sysrq_help() {
	cprintf(LEGACY_COLOUR_WHITE, u"'h' - Help\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'o' - Shutdown\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'b' - Reboot\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'s' - Schedule sleep\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'m' - List memory info\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'t' - List running tasks\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'T' - Show CPU tempature\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'y' - Enable keybord mouse\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'Y' - Disable keybord mouse\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'c' - Simulate a crash\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'C' - Simulate a machine check\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'f' - Show current FPS\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'F' - Unlock FPS\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'e' - List emojis\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'l' - Cycle language\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'i' - Show CPU info\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'n' - Show network interfaces\n");
	cprintf(LEGACY_COLOUR_WHITE, u"'p' - Dump PCI info\n");
}

void sysrq_handle_event(event_t * event) {
	if (event->type != EVENT_KEYBOARD) {
		return;
	}
	kbd_event_t * keyevent = (kbd_event_t *) event;
	keyboard_held_t * held = keyevent->held;
	static int do_sysrq = 0;

	if (held->super && keyevent->keycode == 0x56) {
		do_sysrq = 1;
		return;
	}

	if (keyevent->keycode == 0x2a || keyevent->keycode == 0x36) {
		return;
	}

	if (!held->super || !do_sysrq || keyevent->pressed == 0) {
		do_sysrq = 0;
		return;
	}
	do_sysrq = 0;
	uint16_t chr = event_to_ascii_char(keyevent);
	if (chr == u'\0') {
		return;
	}
	switch (chr) {
		default:
		case u'h':
			sysrq_help();
			return;

		case u'i':
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Vendor: %s\n", cpu_vendor);
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Family: %d\n", cpu_family);
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Model: %d\n", cpu_model_id);
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Stepping: %d\n", cpu_stepping);
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Count: %d\n", cpus);
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Type: %s\n", cputype_decode(cpu_type));
			cprintf(LEGACY_COLOUR_WHITE, u"CPU Name: %s\n", cpu_vendor_id->name);
			return;

		case u'o':
			shutdown();
			halt();
			return;

		case u'b':
			reboot();
			halt();
			return;

		case u's':
			cprintf(LEGACY_COLOUR_WHITE, u"Sleeping in 2 seconds...\n");
			sleep_mode_schedule = ticks + pit_freq;
			return;

		case u'n':
			linked_iterate(network_interfaces, sysrq_dump_interface, 0);
			return;

		case u't':
			linked_iterate(procs, sysrq_dump_process, 0);
			return;

		case u'p':
			linked_iterate(pci_devices, sysrq_dump_pci, 0);
			return;

		case u'j':
			pci_t * last;
			linked_iterate(pci_devices, sysrq_dump_pci_deduped, &last);
			return;

		case u'P':
			printf(u"Personalities:\n");
			for (int i = 0; i < personality_top; i++) {
				personality_t * personality = personality_table[i];
				printf(u" - %w : %s (%d syscalls)\n", personality->type, personality->name, personality->syscall_count);
			}
			return;

		case u'T':
			cprintf(LEGACY_COLOUR_WHITE, u"The CPU tempature is %d°C\n", get_cpu_temp());
			return;

		case u'y':
			// y for Your arrow key moves the cursor now
			keyboard_mouse = 1;
			return;

		case u'Y':
			keyboard_mouse = 0;
			return;

		case u'c':
			asm volatile ("int $1");
			return;

		case u'C':
			asm volatile ("int $18");
			return;

		case u'f':
			cprintf(LEGACY_COLOUR_WHITE, u"Running at %d FPS\n", fps);
			return;

		case u'F':
			locked_fps = !locked_fps;
			cprintf(LEGACY_COLOUR_WHITE, u"%sing FPS\n", locked_fps ? u"Lock" : u"Unlock");
			return;
	}
	return;
}

void sysrq_init() {
	process_t * proc = create_event_zombie(u"sysrqd", sysrq_handle_event);
	proc->system = 1;
	proc->kill = force_alive_kill_handler;
}
