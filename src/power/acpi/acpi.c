#include <stdint.h>
#include <power/acpi/acpi.h>
#include <ports.h>
#include <string.h>
#include <util.h>
#include <stdio.h>

RSDP_t * RSDP = 0;
FADT_t * FADT = 0;
MADT_t * MADT = 0;
SBST_t * SBST = 0;
HPET_t * HPET = 0;
ACPISDTHeader_t * RSDT = 0;
int acpi_working = 0;

void acpi_parse_dt(ACPISDTHeader_t * table) {
	if (memcmp(table->signature, "FACP", 4) == 0) {
		FADT = (FADT_t *) table;
	} else if (memcmp(table->signature, "APIC", 4) == 0) {
		MADT = (MADT_t *) table;
	} else if (memcmp(table->signature, "SBST", 4) == 0) {
		SBST = (SBST_t *) table;
	} else if (memcmp(table->signature, "HPET", 4) == 0) {
		HPET = (HPET_t *) table;
	}
}

void acpi_find() {
	volatile uint32_t mem = 0x000e0000;
	volatile void * p;
	while (mem < 0x000fffff) {
		p = (void *) mem;
		if (memcmp((void *) p, "RSD PTR ", 8) == 0) {
			break;
		}
		mem += 16;
	}
	if (mem != 0x100000) {
		RSDP = (RSDP_t *) mem;
		RSDT = (ACPISDTHeader_t *) RSDP->address;
		uint32_t header = ((uint32_t) RSDT) + sizeof(RSDP_t);
		uint32_t end = ((uint32_t) RSDT) + RSDT->length;
		while (header < end) {
			uint32_t address = *(uint32_t *) header;
			acpi_parse_dt((void *) address);
			header += 4;
		}
	}
}

int acpi_reboot() {
	if (!FADT) {
		return 0;
	}
	return 0;
}

int acpi_shutdown() {
	if (!FADT) {
		return 0;
	}
	return 0;
}

void acpi_init() {
	acpi_find();
	if (!FADT) {
		return;
	}
	outb(FADT->SMI_CommandPort, FADT->AcpiEnable);
	acpi_working = 0;
}
