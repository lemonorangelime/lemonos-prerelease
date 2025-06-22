#include <assert.h>
#include <panic.h>
#include <util.h>

void assert(int expr, int error, void * p) {
	if (!expr) {
		handle_error(error, p);
	}
}