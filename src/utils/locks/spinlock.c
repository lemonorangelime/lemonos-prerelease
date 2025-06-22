#include <memory.h>
#include <util.h>

int * spinlock_create() {
	int * lock = malloc(4);
	*lock = 0;
	return lock;
}

void spinlock_acquire(int * lock) {
	while (*lock) {}
	*lock = 1;
}

void spinlock_release(int * lock) {
	*lock = 0;
}