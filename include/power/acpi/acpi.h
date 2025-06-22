#pragma once

// https://uefi.org/sites/default/files/resources/ACPI%206_2_A_Sept29.pdf

#include <stdint.h>

typedef struct {
	char signature[8];
	uint8_t checksum;
	char OEMID[6];
	uint8_t revision;
	uint32_t address;
	uint32_t length;
	uint64_t xaddress;
	uint8_t xchecksum;
	uint8_t reserved[3];
} __attribute__ ((packed)) RSDP_t;

typedef struct {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} __attribute__ ((packed)) ACPISDTHeader_t;

typedef struct {
	uint8_t AddressSpace;
	uint8_t BitWidth;
	uint8_t BitOffset;
	uint8_t AccessSize;
	uint64_t Address;
} __attribute__ ((packed)) GenericAddressStructure;

typedef struct {
	ACPISDTHeader_t header;
	uint32_t FirmwareCtrl;
	uint32_t Dsdt;
	uint8_t Reserved;
	uint8_t PreferredPowerManagementProfile;
	uint16_t SCI_Interrupt;
	uint32_t SMI_CommandPort;
	uint8_t AcpiEnable;
	uint8_t AcpiDisable;
	uint8_t S4BIOS_REQ;
	uint8_t PSTATE_Control;
	uint32_t PM1aEventBlock;
	uint32_t PM1bEventBlock;
	uint32_t PM1aControlBlock;
	uint32_t PM1bControlBlock;
	uint32_t PM2ControlBlock;
	uint32_t PMTimerBlock;
	uint32_t GPE0Block;
	uint32_t GPE1Block;
	uint8_t PM1EventLength;
	uint8_t PM1ControlLength;
	uint8_t PM2ControlLength;
	uint8_t PMTimerLength;
	uint8_t GPE0Length;
	uint8_t GPE1Length;
	uint8_t GPE1Base;
	uint8_t CStateControl;
	uint16_t WorstC2Latency;
	uint16_t WorstC3Latency;
	uint16_t FlushSize;
	uint16_t FlushStride;
	uint8_t DutyOffset;
	uint8_t DutyWidth;
	uint8_t DayAlarm;
	uint8_t MonthAlarm;
	uint8_t Century;
	uint16_t BootArchitectureFlags;
	uint8_t Reserved2;
	uint32_t Flags;
	GenericAddressStructure ResetReg;
	uint8_t ResetValue;
	uint8_t Reserved3[3];
	uint64_t X_FirmwareControl;
	uint64_t X_Dsdt;
	GenericAddressStructure X_PM1aEventBlock;
	GenericAddressStructure X_PM1bEventBlock;
	GenericAddressStructure X_PM1aControlBlock;
	GenericAddressStructure X_PM1bControlBlock;
	GenericAddressStructure X_PM2ControlBlock;
	GenericAddressStructure X_PMTimerBlock;
	GenericAddressStructure X_GPE0Block;
	GenericAddressStructure X_GPE1Block;
} __attribute__ ((packed)) FADT_t;

typedef struct {
	ACPISDTHeader_t header;
	uint32_t ApicAddr;
	uint32_t flags;
} __attribute__ ((packed)) MADT_t;

typedef struct {
	uint8_t type;
	uint8_t length;
	uint8_t processor;
	uint8_t apicid;
} __attribute__ ((packed)) MADT_APIC_t;

typedef struct {
	ACPISDTHeader_t header;
	uint32_t warningEnergy;
	uint32_t lowEnergy;
	uint32_t criticalEnergy;
} __attribute__ ((packed)) SBST_t;

typedef struct {
	ACPISDTHeader_t header;
	uint8_t revision;
	uint8_t count : 5;
	uint8_t padding : 1;
	uint8_t legacy : 1;
	uint16_t pci_vendor;
	GenericAddressStructure address;
	uint8_t number;
	uint16_t minimum;
	uint8_t protection;
} __attribute__ ((packed)) HPET_t;


enum {
	GAS_MEMORY_MAPPED = 0,
	GAS_PIO,
	GAS_PCI_CONFIG,
	GAS_EMBEDDED,
	GAS_SMB,
	GAS_CMOS,
	GAS_PCI_BAR,
	GAS_IPMI,
	GAS_GPIO,
	GAS_SERIAL,
	GAS_PCC,
};

enum {
	PMP_NONE = 0,
	PMP_DESKTOP,
	PMP_MOBILE,
	PMP_WORKSTATION,
	PMP_SERVER,
	PMP_SOHO,
	PMP_APLLIANCE,
	PMP_PREFORMANCE,
};

enum {
	APIC_LOCAL = 1,
	APIC_IO,
	APIC_IO_OVERRIDE,
	APIC_IO_NMI,
	APIC_LOCAL_NMI,
	APIC_LOCAL_OVERRIDE,
	APIC_LOCALx2 = 9,
};

int acpi_reboot();
int acpi_shutdown();
void acpi_init();

extern RSDP_t * RSDP;
extern FADT_t * FADT;
extern MADT_t * MADT;
extern SBST_t * SBST;
extern HPET_t * HPET;
extern ACPISDTHeader_t * RSDT;

extern uint32_t cpu_count;

extern int acpi_working;