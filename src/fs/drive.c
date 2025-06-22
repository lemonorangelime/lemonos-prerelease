#include <fs/drive.h>

uint32_t lba2chs(uint32_t lba, int heads, int sectors) {
	uint32_t chs = 0;
	int modulo = (heads * sectors);
	chs |= (lba / modulo) << 16;
	chs |= ((lba / sectors) % heads) << 8;
	chs |= ((lba % sectors) + 1);
	return chs;
}
