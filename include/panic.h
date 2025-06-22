#pragma once

#include <stdint.h>
#include <asm.h>

enum {
	MANUAL_PANIC = 0, // somebody manually called panic (and didnt care to specify an error code)
	OUT_OF_MEMORY, // where did it go?
	MEMORY_CORRUPTION, // somebody fucked up memory they shouldnt be touching
	GENERAL_MEMORY, // memory exploded :shrug:
	GENERAL_PROTECTION, // many things
	GENERAL_PAGE, // never gonna happen (paging broke)
	GENERAL_ARITHMATIC, // math exploded
	UNKNOWN_OPCODE, // some IDIOT is executing stuff thats not code
	DEVICE_UNAVAILABLE, // coprocessor doesnt exist but someone tried to use it anyway
	UNKNOWN_FATAL_ERROR, // :shrug:
	SHUTDOWN_FAILURE, // couldnt shutdown
	MULTIBOOT_ERROR, // bootloader doesnt use multiboot or its brokey
	ACPI_FATAL_ERROR, // acpi exploded
	SEGMENTATION_FAULT, // never happens (means something segmentation related exploded)
	GRAPHICS_ERROR, // couldnt initalise graphics
	COPROCESSOR_FATAL, // the coprocessor exploded
	MULTIPROCESSING_FAILURE, // something went wrong switching tasks
	MCE_HARDWARE_FAILURE, // the hardware exploded
	UNSUPPORTED_PROCESSOR, // we dont like your cpu
	DEBUG_EXCEPTION, // debug cpu exception
	THERM_CATASTROPHIC_SHUTDOWN, // your cpu is about to melt
	RECIEVED_64BIT_POINTER,
	SYSTEM_FILE_MISSING, // if you delete a file we need
};

uint32_t draw_panic_screen(uint32_t colour, uint32_t target, uint32_t mask, float divisor);

void panic(int error, void * p);
void mce_panic(int error, void * p);
void sensitive_panic(int error, void * p);
void panic_irq(registers_t regs);
void handle_error(int error, void * p);
uint16_t * error_name(int error);
uint32_t getcr(int cr);
