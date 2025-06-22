#include <personalities.h>

static personality_t personality;

void personality_roadrunner_init() {
	personality_register(&personality);
}

static personality_t personality = {
	PERSONALITY_ROADRUNNER, u"RoadrunnerOS", 0,
	{}
};