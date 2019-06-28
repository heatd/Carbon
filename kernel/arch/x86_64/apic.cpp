/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdint.h>

#include <carbon/x86/apic.h>
#include <carbon/vm.h>
#include <carbon/memory.h>

namespace x86
{

namespace Apic
{

volatile char *ioapic_base = NULL;

uint32_t ReadIoApic(uint32_t reg)
{
	uint32_t volatile *ioapic = (uint32_t volatile*) ioapic_base;
	ioapic[0] = (reg & 0xFF);
	return ioapic[4];
}

void WriteIoApic(uint32_t reg, uint32_t value)
{
	uint32_t volatile *ioapic = (uint32_t volatile*) ioapic_base;
	ioapic[0] = (reg & 0xFF);
	ioapic[4] = value;
}

uint64_t ReadRedirectionEntry(uint32_t pin)
{
	uint64_t ret;
	ret = (uint64_t) ReadIoApic(0x10 + pin * 2);
	ret |= (uint64_t) ReadIoApic(0x10 + pin * 2 + 1) << 32;
	return ret;
}

void WriteRedirectionEntry(uint32_t pin, uint64_t value)
{
	WriteIoApic(0x10 + pin * 2, value & 0x00000000FFFFFFFF);
	WriteIoApic(0x10 + pin * 2 + 1, value >> 32);
}

static int irqs;

void SetPin(bool active_high, bool level, uint32_t pin)
{
	uint64_t entry = 0;
	entry |= irqs + pin;

	if(!active_high)
	{
		/* Active low */
		entry |= IOAPIC_PIN_POLARITY_ACTIVE_LOW;
	}
	if(level)
	{
		entry |= IOAPIC_PIN_TRIGGER_LEVEL;
	}
	WriteRedirectionEntry(pin, entry);
}

void UnmaskPin(uint32_t pin)
{
	/*printk("Unmasking pin %u\n", pin);*/
	uint64_t entry = ReadRedirectionEntry(pin);
	entry &= ~IOAPIC_PIN_MASKED;
	WriteRedirectionEntry(pin, entry);
}

void MaskPin(uint32_t pin)
{
	/*printk("Masking pin %u\n", pin);*/
	uint64_t entry = ReadRedirectionEntry(pin);
	entry |= IOAPIC_PIN_MASKED;
	WriteRedirectionEntry(pin, entry);
}

void EarlyInit()
{
	ioapic_base = (volatile char *) Vm::MmioMap(&kernel_address_space,
			IOAPIC_BASE_PHYS, 0, PAGE_SIZE, VM_PROT_WRITE);

	assert(ioapic_base != nullptr);
}

}
}