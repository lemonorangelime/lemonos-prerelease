#include <personalities.h>

static personality_t personality;

void personality_templeos_init() {
	personality_register(&personality);
}

static personality_t personality = {
	PERSONALITY_TEMPLEOS, u"TempleOS", 0,
	{}
};