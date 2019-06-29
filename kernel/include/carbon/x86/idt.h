/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdlib.h>
#include <stdint.h>

#include <carbon/interrupt.h>

struct idt_ptr
{
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

typedef struct idt_ptr idt_ptr_t;

struct IDT_entry
{
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;/* unused, set to 0 */
	uint8_t type_attr;
	uint16_t offset_high;
	uint32_t offset_top;
	uint32_t res;
}__attribute__((packed));

typedef struct IDT_entry idt_entry_t;

void idt_create_descriptor(uint8_t entry, uint64_t offset, uint16_t selector, uint8_t flags);
void idt_set_system_gate(uint8_t entry, uint64_t offset, uint16_t selector, uint8_t flags);
void idt_load();
void x86_init_exceptions(void);
void x86_reserve_vector(Interrupt::InterruptVector vector, void (*handler)());

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();
extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();
extern "C" void irq16();
extern "C" void irq17();
extern "C" void irq18();
extern "C" void irq19();
extern "C" void irq20();
extern "C" void irq21();
extern "C" void irq22();
extern "C" void irq23();
extern "C" void syscall_ENTRY64_int();
extern "C" void apic_spurious_irq();