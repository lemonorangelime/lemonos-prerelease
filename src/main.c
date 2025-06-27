#include <main.h>
#include <memory.h>
#include <multiboot.h>
#include <assert.h>
#include <graphics/graphics.h>
#include <panic.h>
#include <stdio.h>
#include <version.h>
#include <fpu.h>
#include <net/net.h>
#include <power/acpi/acpi.h>
#include <string.h>
#include <util.h>
#include <interrupts/apic.h>
#include <interrupts/irq.h>
#include <pit.h>
#include <drivers/input/mouse.h>
#include <drivers/input/keyboard.h>
#include <bus/pci.h>
#include <input/input.h>
#include <multitasking.h>
#include <fs/ide.h>
#include <fs/fdc.h>
#include <cpuspeed.h>
#include <input/layout.h>
#include <sysrq.h>
#include <syscall.h>
#include <drivers/ether/rtl8139.h>
#include <drivers/ether/pcnet32.h>
#include <boot/shell.h>
#include <drivers/thermal/thermal.h>
#include <cpuid.h>
#include <power/sleep.h>
#include <power/perf.h>
#include <errors/mce.h>
#include <uname.h>
#include <boot/initrd.h>
#include <bin_formats/elf.h>
#include <simd.h>
#include <vm86/v8086.h>
#include <net/icmp.h>
#include <net/socket.h>
#include <net/udp.h>
#include <net/arp.h>
#include <drivers/vga/svga.h>
#include <drivers/ether/ne2k.h>
#include <graphics/font.h>
#include <drivers/input/vmouse.h>
#include <bus/usb.h>
#include <bios/smbios.h>
#include <drivers/vdebugger/vdebugger.h>
#include <personalities.h>
#include <drivers/3d/s3virge.h>
#include <drivers/misc/vbox.h>
#include <drivers/vga/bochs.h>
#include <drivers/rng/rng.h>
#include <graphics/gpu.h>

void main2() {
	while (1) {
		yield();
	}
}

int main(uint32_t eax, uint32_t ebx) {
	disable_interrupts();
	cpuid_init();
	//perf_init();
	//mce2_init();
	simd_init();
	assert(parse_multiboot(eax, ebx), MULTIBOOT_ERROR, 0);
	memory_init();
	irq_init();
	pit_init(1000);
	layout_init();
	//ide_init();
	memset((void *) 0x800, 0, 2048);
	enable_interrupts();
	keyboard_init();
	mouse_init();
	ksystem(toustrdup(multiboot_header->cmdline));
	gfx_init();
	initrd_init();
	if (cpu_type == CPU_APPLE || cpu_type == CPU_WINDOWS) {
		panic(UNSUPPORTED_PROCESSOR, 0);
		halt();
	}
	cprintf(LEGACY_COLOUR_WHITE, u"Kernel Loaded \ue029\ue02a\n");
	cprintf(LEGACY_COLOUR_WHITE, u"\n");
	cprintf(LEGACY_COLOUR_WHITE, u"You are using LemonOS v%d.%d.%d.%d (%s)", ver_edition, ver_major, ver_minor, ver_patch, os_name16);
	cprintf(LEGACY_COLOUR_WHITE, u"\n");
	//thermal_init();
	//cpuspeed_measure();
	//acpi_init();
	//apic_init();
	if (madt_broken) {
		cprintf(LEGACY_COLOUR_RED, u"Firmware contains broken ACPI tables, aborted booting other CPUs\n");
	}
	cprintf(LEGACY_COLOUR_WHITE, u"CPU Frequency: %uMHz\n", cpu_mhz);
	cprintf(LEGACY_COLOUR_WHITE, u"PIT Frequency: %dHz\n", pit_freq);
	cprintf(LEGACY_COLOUR_WHITE, u"FPU Level: %d\n", fpu_level);
	cprintf(LEGACY_COLOUR_WHITE, u"SSE Level: %d\n", sse_level);
	cprintf(LEGACY_COLOUR_WHITE, u"AVX Level: %d\n", avx_level);

	cprintf(LEGACY_COLOUR_WHITE, u"CPU Target Tempature: %u°C\n", therm_target);
	cprintf(LEGACY_COLOUR_WHITE, u"CPU Tempature: %u°C\n", get_cpu_temp());

	fdc_init();
	personality_init();
	multitasking_init();
	v8086_init();
	event_init();
	net_init();
	gpu_init();
	pid_top = 3;
	gfx_late_init(); // gfx
	mouse_late_init(); // moused init
	// memory_late_init();
	sysrq_init();
	uname_init();
	init_syscall(); // init syscall interface
	usb_init(); // usb
	s3virge_init(); // S3 ViRGE 3D Accelerator
	rtl8139_init(); // init rtl8139 network driver
	pcnet32_init(); // init pcnet32 network driver
	ne2k_init(); // init ne2000 network driver
	svga_init(); // init svga graphics driver
	bochsvga_init(); // init bochs vga controller
	vmouse_init(); // init virtual mouse driver
	vdebugger_init(); // init virtual debugger driver
	vbox_init(); // init virtualbox guest interface
	rng_init();
	pci_probe();
	smbios_init();
	printf(u"multicore stack: %r\n", *multicpu_stack);
	printf(u"cpus booted: %d\n", *cpus_booted);
	printf(u"init done\n");
	arp_init();
	icmp_init();
	socket_init();
	udp_init();
	yield();
	yield();
	yield();

	printf(u"using %dMb of installed %dMb RAM\n", used_memory / 1024000, heap_length / 1024000);

	protected_lock = 0;
	(*get_current_process())->killed = 1;
	while (1) {
		yield();
	}
}
