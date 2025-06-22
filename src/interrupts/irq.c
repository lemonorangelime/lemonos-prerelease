#include <interrupts/irq.h>
#include <asm.h>
#include <input/input.h>
#include <multitasking.h>
#include <string.h>
#include <stdint.h>
#include <ports.h>
#include <panic.h>
#include <errors/mce.h>

// pretty plagarised so dont bully me for this being bad

isr_t interrupt_handlers[256];
idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;
gdt_entry_t gdt_entries[5];
gdt_ptr_t gdt_ptr;
int sensitive_mode = 0;

void irq_ack(uint32_t number) {
	if (number >= 40) {
		outb(0xa0, 0x20);
	}
	outb(0x20, 0x20);
}

void irq_handler(registers_t * regs) {
	if (interrupt_handlers[regs->int_no] == 0) {
		irq_ack(regs->int_no);
		return;
	}
	isr_t handler = interrupt_handlers[regs->int_no];
	handler(regs);
	irq_ack(regs->int_no);
}

void isr_handler(registers_t * regs) {
	if (interrupt_handlers[regs->int_no] == 0) {
		return;
	}
	isr_t handler = interrupt_handlers[regs->int_no];
	handler(regs);
}

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
	gdt_entries[num].base_low = (base & 0xffff);
	gdt_entries[num].base_middle = (base >> 16) & 0xff;
	gdt_entries[num].base_high = (base >> 24) & 0xff;
	gdt_entries[num].limit_low = (limit & 0xffff);
	gdt_entries[num].granularity = (limit >> 16) & 0x0F;
	gdt_entries[num].granularity |= gran & 0xF0;
	gdt_entries[num].access = access;
}

void idt_set_gate(uint16_t num, uint64_t base, uint16_t sel, uint8_t flags) {
	idt_entries[num].base_low = base & 0xffff;
	idt_entries[num].base_high = (base >> 16) & 0xffff;
	idt_entries[num].sel = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags = flags;
}

void idt_init() {
	idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
	idt_ptr.base  = (uint32_t) &idt_entries;
	memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);
	outb(0x20, 0x11);
	outb(0xa0, 0x11);
	outb(0x21, 0x20);
	outb(0xa1, 0x28);
	outb(0x21, 0x04);
	outb(0xa1, 0x02);
	outb(0x21, 0x01);
	outb(0xa1, 0x01);
	outb(0x21, 0x00);
	outb(0xa1, 0x00);
	idt_set_gate(0, (uint32_t) isr0, 0x08, 0x8e);
	idt_set_gate(1, (uint32_t) isr1, 0x08, 0x8e);
	idt_set_gate(2, (uint32_t) isr2, 0x08, 0x8e);
	idt_set_gate(3, (uint32_t) isr3, 0x08, 0x8e);
	idt_set_gate(4, (uint32_t) isr4, 0x08, 0x8e);
	idt_set_gate(5, (uint32_t) isr5, 0x08, 0x8e);
	idt_set_gate(6, (uint32_t) isr6, 0x08, 0x8e);
	idt_set_gate(7, (uint32_t) isr7, 0x08, 0x8e);
	idt_set_gate(8, (uint32_t) isr8, 0x08, 0x8e);
	idt_set_gate(9, (uint32_t) isr9, 0x08, 0x8e);
	idt_set_gate(10, (uint32_t) isr10, 0x08, 0x8e);
	idt_set_gate(11, (uint32_t) isr11, 0x08, 0x8e);
	idt_set_gate(12, (uint32_t) isr12, 0x08, 0x8e);
	idt_set_gate(13, (uint32_t) isr13, 0x08, 0x8e);
	idt_set_gate(14, (uint32_t) isr14, 0x08, 0x8e);
	idt_set_gate(15, (uint32_t) isr15, 0x08, 0x8e);
	idt_set_gate(16, (uint32_t) isr16, 0x08, 0x8e);
	idt_set_gate(17, (uint32_t) isr17, 0x08, 0x8e);
	idt_set_gate(18, (uint32_t) isr18, 0x08, 0x8e);
	idt_set_gate(19, (uint32_t) isr19, 0x08, 0x8e);
	idt_set_gate(20, (uint32_t) isr20, 0x08, 0x8e);
	idt_set_gate(21, (uint32_t) isr21, 0x08, 0x8e);
	idt_set_gate(22, (uint32_t) isr22, 0x08, 0x8e);
	idt_set_gate(23, (uint32_t) isr23, 0x08, 0x8e);
	idt_set_gate(24, (uint32_t) isr24, 0x08, 0x8e);
	idt_set_gate(25, (uint32_t) isr25, 0x08, 0x8e);
	idt_set_gate(26, (uint32_t) isr26, 0x08, 0x8e);
	idt_set_gate(27, (uint32_t) isr27, 0x08, 0x8e);
	idt_set_gate(28, (uint32_t) isr28, 0x08, 0x8e);
	idt_set_gate(29, (uint32_t) isr29, 0x08, 0x8e);
	idt_set_gate(30, (uint32_t) isr30, 0x08, 0x8e);
	idt_set_gate(31, (uint32_t) isr31, 0x08, 0x8e);
	idt_set_gate(32, (uint32_t) irq0, 0x08, 0x8e);
	idt_set_gate(33, (uint32_t) irq1, 0x08, 0x8e);
	idt_set_gate(34, (uint32_t) irq2, 0x08, 0x8e);
	idt_set_gate(35, (uint32_t) irq3, 0x08, 0x8e);
	idt_set_gate(36, (uint32_t) irq4, 0x08, 0x8e);
	idt_set_gate(37, (uint32_t) irq5, 0x08, 0x8e);
	idt_set_gate(38, (uint32_t) irq6, 0x08, 0x8e);
	idt_set_gate(39, (uint32_t) irq7, 0x08, 0x8e);
	idt_set_gate(40, (uint32_t) irq8, 0x08, 0x8e);
	idt_set_gate(41, (uint32_t) irq9, 0x08, 0x8e);
	idt_set_gate(42, (uint32_t) irq10, 0x08, 0x8e);
	idt_set_gate(43, (uint32_t) irq11, 0x08, 0x8e);
	idt_set_gate(44, (uint32_t) irq12, 0x08, 0x8e);
	idt_set_gate(45, (uint32_t) irq13, 0x08, 0x8e);
	idt_set_gate(46, (uint32_t) irq14, 0x08, 0x8e);
	idt_set_gate(47, (uint32_t) irq15, 0x08, 0x8e);
	idt_set_gate(48, (uint32_t) irq16, 0x08, 0x8e);
	idt_set_gate(127, (uint32_t) isr127, 0x08, 0x8e);
	idt_set_gate(128, (uint32_t) isr128, 0x08, 0x8e);
	idt_flush((uint32_t) &idt_ptr);
}

void gdt_init() {
	gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
	gdt_ptr.base  = (uint32_t) &gdt_entries[0];
	gdt_set_gate(0, 0, 0, 0, 0);
	gdt_set_gate(1, 0, 0xffffffff, 0x9a, 0xcf);
	gdt_set_gate(2, 0, 0xffffffff, 0x92, 0xcf);
	gdt_set_gate(3, 0, 0xffffffff, 0xfa, 0xcf);
	gdt_set_gate(4, 0, 0xffffffff, 0xf2, 0xcf);
	gdt_flush((uint32_t) &gdt_ptr);
}

void irq_set_handler(uint8_t n, isr_t handler) {
	interrupt_handlers[n] = handler;
}

void irq_null(registers_t * regs) {}

void irq_error_prehandle(registers_t * regs) {
	if (sensitive_mode) {
		sensitive_panic(UNKNOWN_FATAL_ERROR, regs);
	}
}

void irq_division_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(GENERAL_ARITHMATIC, regs);
}

void irq_debug_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	panic(DEBUG_EXCEPTION, regs);
}

void irq_opcode_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(UNKNOWN_OPCODE, regs);
}

void irq_device_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	mce_panic(DEVICE_UNAVAILABLE, regs);
}

void irq_double_fault_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(UNKNOWN_FATAL_ERROR, regs);
}

void irq_segfault_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(SEGMENTATION_FAULT, regs);
}

void irq_gp_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(GENERAL_PROTECTION, regs);
}

void irq_fpu_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(COPROCESSOR_FATAL, regs);
}

void irq_mce_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	mce_panic(MCE_HARDWARE_FAILURE, regs);
}

void irq_page_fault_error(registers_t * regs) {
	irq_ack(regs->int_no);
	irq_error_prehandle(regs);
	multitasking_handle_exception(GENERAL_PAGE, regs);
}

void irq_init() {
	gdt_init();
	idt_init();
	memset(&interrupt_handlers, 0, sizeof(isr_t) * 256);
	irq_set_handler(0, &irq_division_error);
	irq_set_handler(1, &irq_debug_error);
	irq_set_handler(2, &irq_device_error);
	irq_set_handler(6, &irq_opcode_error);
	irq_set_handler(7, &irq_device_error);
	irq_set_handler(8, &irq_double_fault_error);
	irq_set_handler(11, &irq_segfault_error);
	irq_set_handler(12, &irq_segfault_error);
	irq_set_handler(13, &irq_gp_error);
	irq_set_handler(14, &irq_page_fault_error);
	irq_set_handler(16, &irq_fpu_error);
	irq_set_handler(18, &irq_mce_error);
	irq_set_handler(19, &irq_fpu_error);
}
