#include <memory.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <linked.h>
#include <drivers/rng/tpm/tpm.h>
#include <drivers/rng/rng.h>
#include <drivers/rng/algorithms/mt19937.h>

linked_t * rng_backends;
int rng_id = 0;

rng_backend_t * rng_create_backend(uint16_t * name, rng_generator_t generate, rng_seeder_t seed) {
	rng_backend_t * backend = malloc(sizeof(rng_backend_t));
	backend->name = ustrdup(name);
	backend->id = rng_id++;
	backend->selectable = 0;
	backend->generate = generate;
	backend->seed = seed;
	return backend;
}

void rng_register_backend(rng_backend_t * backend) {
	rng_backends = linked_add(rng_backends, backend);
}

rng_backend_t * rng_make_selectable(rng_backend_t * backend) {
	backend->selectable = 1;
}

rng_backend_t * rng_auto_select() {
	linked_iterator_t iterator = {rng_backends};
	linked_t * node = linked_step_iterator(&iterator);
	while (node) {
		rng_backend_t * backend = node->p;
		if (backend->selectable) {
			return backend;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

rng_backend_t * rng_find(int id) {
	linked_iterator_t iterator = {rng_backends};
	linked_t * node = linked_step_iterator(&iterator);
	while (node) {
		rng_backend_t * backend = node->p;
		if (backend->id == id) {
			return backend;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

rng_backend_t * rng_find_by_name(uint16_t * name) {
	linked_iterator_t iterator = {rng_backends};
	linked_t * node = linked_step_iterator(&iterator);
	while (node) {
		rng_backend_t * backend = node->p;
		if (ustrcmp(backend->name, name) == 0) {
			return backend;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

size_t rng_generate(rng_backend_t * backend, void * buffer, size_t size) {
	if (!backend && !(backend = rng_auto_select())) {
		return 0;
	}
	return backend->generate(backend, buffer, size);
}

size_t rng_seed(rng_backend_t * backend, void * buffer, size_t size) {
	if (!backend && !(backend = rng_auto_select())) {
		return 0;
	}
	return backend->seed(backend, buffer, size);
}

void rng_init() {
	mt19937_init();
	tpm_init();
}
