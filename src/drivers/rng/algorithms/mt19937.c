#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <arrays.h>
#include <stdio.h>
#include <drivers/rng/rng.h>
#include <drivers/rng/algorithms/mt19937.h>

// entirely plagarised

#define STATE_SIZE 624
#define MIDDLE 397
#define INIT_SHIFT 30
#define INIT_FACT 1812433253
#define TWIST_MASK 0x9908b0df
#define SHIFT1 11
#define MASK1 0xffffffff
#define SHIFT2 7
#define MASK2 0x9d2c5680
#define SHIFT3 15
#define MASK3 0xefc60000
#define SHIFT4 18

#define LOWER_MASK 0x7fffffff
#define UPPER_MASK (~(uint16_t) LOWER_MASK)

static uint16_t state[STATE_SIZE];
static size_t index = STATE_SIZE + 1;

static void seed(uint16_t s) {
	index = STATE_SIZE;
	state[0] = s;
	for (size_t i = 1; i < STATE_SIZE; i++) {
		state[i] = (INIT_FACT * (state[i - 1] ^ (state[i - 1] >> INIT_SHIFT))) + i;
	}
}

static void twist(void) {
	for (size_t i = 0; i < STATE_SIZE; i++) {
		uint16_t x = (state[i] & UPPER_MASK) | (state[(i + 1) % STATE_SIZE] & LOWER_MASK);
		x = (x >> 1) ^ (x & 1 ? TWIST_MASK : 0);
		state[i] = state[(i + MIDDLE) % STATE_SIZE] ^ x;
	}
	index = 0;
}

static uint16_t random(void) {
    if (index >= STATE_SIZE) {
        twist();
    }

    uint16_t y = state[index];
    y ^= (y >> SHIFT1) & MASK1;
    y ^= (y << SHIFT2) & MASK2;
    y ^= (y << SHIFT3) & MASK3;
    y ^= y >> SHIFT4;

    index++;
    return y;
}

size_t mt19937_generate(rng_backend_t * backend, void * buffer, size_t size) {
	uint16_t word = 0;
	bytes_iterator_t iterator = {buffer, &word, size, 2};
	uint16_t * p = buffer;
	int last = size;
	while (1) {
		p = bytes_step_iterator(&iterator);
		if (!p) {
			break;
		}
		if (last < 2) {
			uint8_t * p2 = (uint8_t *) p;
			*p2 = random() & 0xff;
			last = iterator.size;
			continue;
		}
		*p = random();
		last = iterator.size;
	}
	return size;
}

size_t mt19937_seed(rng_backend_t * backend, void * buffer, size_t size) {
	uint16_t word = 0;
	uint16_t seedval = 0;
	bytes_iterator_t iterator = {buffer, &word, size, 2};
	void * p = buffer;
	while (p) {
		p = bytes_step_iterator(&iterator);
		seedval ^= word;
	}
	seed(seedval);
	return size;

}

void mt19937_init() {
	rng_backend_t * backend = rng_create_backend(u"MT19937", mt19937_generate, mt19937_seed);
	rng_make_selectable(backend);
	rng_register_backend(backend);
}
