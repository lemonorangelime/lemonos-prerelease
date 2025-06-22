#include <stdint.h>
#include <string.h>
#include <drivers/rng/tpm/tpmdev.h>

tpm_vendor_t tpm_vendors[] = {
	{0x0000, u"UNKNOWN"},
	{0x0302, u"Microchip"},
	{0x1022, u"AMD"},
	{0x6688, u"Ant"},
	{0x1114, u"Atmel"},
	{0x14e4, u"Broadcom"},
	{0xc5c0, u"Cisco"},
	{0x6666, u"Google"},
	{0x1ae0, u"Google"},
	{0x1590, u"HPE"},
	{0x8888, u"Huawei"},
	{0x1014, u"IBM"},
	{0x15d1, u"Infineon"},
	{0x8086, u"Intel"},
	{0x17aa, u"Lenovo"},
	{0x1414, u"Microsoft"},
	{0x100b, u"NatSemiCo"},
	{0x1b4e, u"NationZ"},
	{0x9999, u"NSING"},
	{0x1050, u"Nuvoton"},
	{0x1011, u"Qualcomm"},
	{0x144d, u"Samsung"},
	{0x5ece, u"SecEdge"},
	{0x1055, u"SMSC"},
	{0x104a, u"STMicroelectronics"},
	{0x5453, u"STMicroelectronics"},
	{0x104c, u"Texas Instruments"},
	{0x2406, u"Wisekey"}
};

tpm_dev_t tpm_devs[] = {
	{0x0000, 0x0000, u"UNKNOWN"},
	{0x0302, 0x0005, u"ATTPM20P"},
	{0x0302, 0x0006, u"ATTPM20P"},
	{0x1022, 0x0000, u"AMD"},
	{0x1114, 0x3204, u"AT97SC3204"},
	{0x6666, 0x504a, u"Ti50 DT"},
	{0x6666, 0x5066, u"Ti50 OT"},
	{0x6666, 0x0028, u"Cr50 H1"},
	{0x1ae0, 0x0028, u"Cr50 H1"},
	{0x1014, 0x0001, u"Generic TPM"},
	{0x15d1, 0x0006, u"SLD9630 TT 1.1"},
	{0x15d1, 0x000b, u"SLB9635 TT 1.2"},
	{0x15d1, 0x001b, u"SLB9670 TT 2.0"},
	{0x1050, 0x00fe, u"NPCT420AA"},
	{0x104a, 0x0000, u"ST33ZP24"},
	{0x5453, 0x2e4d, u"Unidentified TPM"},
};

int tpm_vendors_count = sizeof(tpm_vendors) / sizeof(tpm_vendor_t);
int tpm_devs_count = sizeof(tpm_devs) / sizeof(tpm_dev_t);

uint16_t tpm_search_vid(uint16_t * name) {
	for (int i = 0; i < tpm_vendors_count; i++) {
		if (ustrcmp(tpm_vendors[i].name, name) == 0) {
			return tpm_vendors[i].vid;
		}
	}
	return 0;
}

uint16_t * tpm_search_vendor(uint16_t vid) {
	for (int i = 0; i < tpm_vendors_count; i++) {
		if (tpm_vendors[i].vid == vid) {
			return tpm_vendors[i].name;
		}
	}
	return tpm_vendors[0].name;
}

uint16_t * tpm_search_device(uint16_t vid, uint16_t did) {
	for (int i = 0; i < tpm_devs_count; i++) {
		if (tpm_devs[i].vid == vid) {
			return tpm_devs[i].name;
		}
	}
	return tpm_devs[0].name;
}
