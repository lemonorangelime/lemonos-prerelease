#include <personalities.h>

static personality_t personality;

void personality_nt_init() {
	personality_register(&personality);
}

static personality_t personality = {
	PERSONALITY_NT, u"WinXP", 0,
	{}
};