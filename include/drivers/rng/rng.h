#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct rng_backend rng_backend_t;

typedef size_t (* rng_generator_t)(rng_backend_t * backend, void *, size_t);
typedef size_t (* rng_seeder_t)(rng_backend_t * backend, void *, size_t);

typedef struct rng_backend {
	int id;
	uint16_t * name;
	rng_generator_t generate;
	rng_seeder_t seed;
	int selectable;
	void * priv;
} rng_backend_t;

rng_backend_t * rng_create_backend(uint16_t * name, rng_generator_t generate, rng_seeder_t seed);
void rng_register_backend(rng_backend_t * backend);
rng_backend_t * rng_make_selectable(rng_backend_t * backend);
rng_backend_t * rng_auto_select();
rng_backend_t * rng_find(int id);
rng_backend_t * rng_find_by_name(uint16_t * name);
size_t rng_generate(rng_backend_t * backend, void * buffer, size_t size);
size_t rng_seed(rng_backend_t * backend, void * buffer, size_t size);
void rng_init();