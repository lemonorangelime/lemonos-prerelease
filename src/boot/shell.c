// Stupid beta loser """shell""" for the grub command line

#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <boot/shell.h>
#include <stdio.h>
#include <drivers/input/keyboard.h>
#include <graphics/graphics.h>
#include <interrupts/apic.h>
#include <errors/mce.h>
#include <pit.h>
#include <interrupts/irq.h>
#include <net/net.h>
#include <bus/usb.h>

static uint16_t * skip_whitespace(uint16_t * text) {
	while (isspace(*text)) {
		text++;
	}
	return text;
}

static void cut_end(uint16_t * text) {
	while (*text++) {}
	while (isspace(*text--)) {}
	*++text = 0;
}

static int count_args(uint16_t * text) {
	int c;
	while (*text++) {
		c += *text == u' ';
	}
	return c;
}

static uint16_t ** allocate_array(int c) {
	return calloc(c + 2, 4);
}

static int assign_array(uint16_t ** array, uint16_t * text) {
	int i = 1;
	array[0] = text;
	while (*text++) {
		if (*text == u' ') {
			*text++ = 0;
			array[i++] = text;
			text = skip_whitespace(text);
			text++;
		}
	}
	return i;
}

cmd_t * find_cmd(uint16_t * text) {
	int i = cmd_len;
	while (i--) {
		if (ustrcmp(cmds[i].text, text) == 0) {
			return &cmds[i];
		}
	}
	return 0;
}

int ksystem(uint16_t * cmd) {
	uint16_t ** array;
	void * oldcmd = cmd;
	int args, len;
	cmd_t * command;
	cmd = ustrdup(cmd);
	cmd = skip_whitespace(cmd);
	cut_end(cmd);
	args = count_args(cmd);
	array = allocate_array(args);
	len = assign_array(array, cmd);
	if (command = find_cmd(cmd)) {
		command->main(len, array);
	}
	free(array);
	free(cmd);
	free(oldcmd);
	return -1;
}

int echo_command(int argc, uint16_t ** argv) {
	for (int i = 1; i < argc; i++) {
		printf(u"%s ", argv[i]);
	}
	printf(u"\n");
}

int keyboard_mouse_command(int argc, uint16_t ** argv) {
	keyboard_mouse = 1;
}

int emulate_mice_command(int argc, uint16_t ** argv) {
	keyboard_mouse = 1;
}

int unlock_fps_command(int argc, uint16_t ** argv) {
	locked_fps = 0;
}

int fastmode_command(int argc, uint16_t ** argv) {
	locked_fps = 0;
}

int nomce_command(int argc, uint16_t ** argv) {
	ignore_mce = 1;
}

int setpit_command(int argc, uint16_t ** argv) {
	if (argc < 2) {
		return - 1;
	}
	pit_disable();
	pit_init(ustrtol(argv[1]));
}

int onefb_command(int argc, uint16_t ** argv) {
	gfx_one_fb = 1;
}

int nofeatures_command(int argc, uint16_t ** argv) {
	multicore_enabled = 0;
	network_enabled = 0;
	usb_enabled = 0;
	gfx_one_fb = 1;
}

int vnopanic_command(int argc, uint16_t ** argv) {
	sensitive_mode = 1;
}

cmd_t cmds[] = {
	{u"echo", echo_command},
	{u"keyboard_mouse", keyboard_mouse_command},
	{u"emulate_mice", emulate_mice_command},
	{u"unlock_fps", unlock_fps_command},
	{u"fastmode", fastmode_command},
	{u"nomce", nomce_command},
	{u"setpit", setpit_command},
	{u"onefb", onefb_command},
	{u"nofeatures", nofeatures_command},
	{u"vnopanic", vnopanic_command}
};

size_t cmd_len = sizeof(cmds) / sizeof(cmds[0]);
