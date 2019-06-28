/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdint.h>

#ifndef _CARBON_APIC_H
#define _CARBON_APIC_H

#define IOAPIC_BASE_PHYS 0xFEC00000
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define IOAPIC_PIN_DESTINATION_MODE		(1 << 11)
#define IOAPIC_PIN_DELIVERY_STATUS		(1 << 12)
#define IOAPIC_PIN_POLARITY_ACTIVE_LOW		(1 << 13)
#define IOAPIC_PIN_TRIGGER_LEVEL		(1 << 15)
#define IOAPIC_PIN_MASKED			(1 << 16)

#define LAPIC_EOI	0xB0
#define LAPIC_TSKPRI	0x80
#define LAPIC_ICR	0x300
#define LAPIC_IPIID	0x310
#define LAPIC_LVT_TIMER	0x320
#define LAPIC_PERFCI	0x340
#define LAPIC_LI0	0x350
#define LAPIC_LI1	0x360
#define LAPIC_ERRINT	0x370
#define LAPIC_SPUINT	0xF0
#define LAPIC_TIMER_DIV	0x3E0
#define LAPIC_TIMER_INITCNT 0x380
#define LAPIC_TIMER_CURRCNT 0x390
#define LAPIC_TIMER_IVT_MASK 0x10000
#define LAPIC_LVT_TIMER_MODE_PERIODIC 0x20000
#define APIC_DEFAULT_SPURIOUS_IRQ 15

namespace x86
{

namespace Apic
{

constexpr unsigned int NumPins = 24;

void EarlyInit();
void MaskPin(uint32_t pin);
void UnmaskPin(uint32_t pin);

}

}

#endif