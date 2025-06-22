#pragma once

#include <asm.h>

typedef struct {
        uint16_t base_low;
        uint16_t sel;
        uint8_t always0;
        uint8_t flags;
        uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
        uint16_t base_low;
        uint16_t sel;
        uint8_t always0;
        uint8_t flags;
        uint16_t base_high;
        uint32_t base_higher;
		uint32_t zero;
} __attribute__((packed)) long_idt_entry_t;

typedef struct {
        uint16_t limit;
        uint64_t base;
} __attribute__((packed)) idt_ptr_t;

typedef struct {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t base_middle;
        uint8_t access;
        uint8_t granularity;
        uint8_t base_high;
} __attribute__ ((packed)) gdt_entry_t;

// 0x0000ffff 00cf9a00

// 0xffff 0x0000 0x00 0x9a 0xcf 0x00

typedef struct {
        uint16_t limit;
        uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef void (*isr_t)(registers_t *);

extern isr_t interrupt_handlers[256];
extern idt_entry_t idt_entries[256];
extern idt_ptr_t idt_ptr;
extern gdt_entry_t gdt_entries[5];
extern gdt_ptr_t gdt_ptr;
extern int sensitive_mode;

void irq_handler(registers_t * regs);
void isr_handler(registers_t * regs);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void idt_set_gate(uint16_t num, uint64_t base, uint16_t sel, uint8_t flags);
void idt_init();
void gdt_init();
void irq_set_handler(uint8_t n, isr_t handler);
void irq_init();
void irq_null(registers_t * regs);
void irq_ack(uint32_t number);

extern void gdt_flush(uint32_t);
extern void idt_flush(uint32_t);

extern void fast_pit_irq();

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void irq16();
extern void isr127();
extern void isr128();
