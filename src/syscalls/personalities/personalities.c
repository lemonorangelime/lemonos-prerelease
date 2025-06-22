#include <personalities.h>
#include <personalities/linux/syscall.h>
#include <personalities/lemonos/syscall.h>
#include <personalities/roadrunner/syscall.h>
#include <personalities/nt/syscall.h>
#include <personalities/templeos/syscall.h>
#include <stdint.h>
#include <string.h>
#include <multitasking.h>

int personality_top = 0;

void personality_init() {
	personality_linux_init();
	personality_lemonos_init();
	personality_roadrunner_init();
	personality_nt_init();
	personality_templeos_init();
}

personality_t * get_personality_by_name(uint16_t * name) {
	for (int i = 0; i < PERSONALITIES; i++) {
		personality_t * personality = personality_table[i];
		if (ustrcmp(personality->name, name)) {
			return personality;
		}
	}
	return NULL;
}

personality_t * get_personality_by_type(int type) {
	for (int i = 0; i < PERSONALITIES; i++) {
		personality_t * personality = personality_table[i];
		if (personality->type == type) {
			return personality;
		}
	}
	return NULL;
}

personality_t * get_current_personality() {
	return (*get_current_process())->personality;
}

void set_current_personality(personality_t * personality) {
	(*get_current_process())->personality = personality;
}

void personality_register(personality_t * personality) {
	personality_table[personality_top++] = personality;
}

personality_t * personality_table[PERSONALITIES];