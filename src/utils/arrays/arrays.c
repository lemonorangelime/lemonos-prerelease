#include <string.h>
#include <arrays.h>

void bytes_shift(bytes_iterator_t * iterator, int step) {
	memcpy(iterator->output, iterator->input, step);
	iterator->input += step;
	iterator->size -= step;
}

void * bytes_step_iterator(bytes_iterator_t * iterator) {
	void * original = iterator->input;
	if (iterator->size == 0) {
		return NULL;
	}
	if (iterator->step >= iterator->size) {
		memset(iterator->output, 0, iterator->step); // throw away some performance
		bytes_shift(iterator, iterator->size);
		return original;
	}
	bytes_shift(iterator, iterator->step);
	return original;
}