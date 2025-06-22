#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <bios/smbios.h>

smbios_anchor_t * smbios_find_anchor() {
	char * p = (char *) 0x0f0000;
	char * end = (char *) 0x100000;
	while (p < end) {
		if (memcmp(p, "_SM_", 4) == 0) {
			return (smbios_anchor_t *) p;
		}
		p += 16;
	}
	return NULL;
}

int smbios_length(smbios_header_t * header) {
	char * string = ((char *) header) + header->size;
	return header->size + strlen(string) + 1;
}

smbios_header_t * smbios_step_iterator(smbios_iterator_t * iterator) {
	smbios_anchor_t * anchor = iterator->anchor;
	uint32_t next = ((uint32_t) iterator->header) + smbios_length(iterator->header);
	uint32_t last = (((uint32_t) anchor->table) + anchor->total_size);
	iterator->header = (smbios_header_t *) next;
	if (next >= last || iterator->header->size == 0) {
		return NULL;
	}
	return iterator->header;
}

void smbios_init() {
	smbios_anchor_t * anchor = smbios_find_anchor();
	// printf(u"SMBIOS: %d.%d (rev %d)\n", anchor->version_major, anchor->version_minor, anchor->revision);
	return;
	smbios_iterator_t iterator = {anchor, anchor->table};
	smbios_header_t * header = anchor->table;
	while (header) {
		//printf(u"SMBIOS: %r %r\n", header->type, header->handle);
		header = smbios_step_iterator(&iterator);
	}
}
