/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_APIC_H
#define _CARBON_APIC_H

#include <stdint.h>

#include <carbon/interrupt.h>
#include <carbon/irq.h>

#define IOAPIC_BASE_PHYS 0xFEC00000
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define IOAPIC_REG_REDIRECTION_BASE		(0x10)

#define IOAPIC_PIN_DESTINATION_MODE		(1 << 11)
#define IOAPIC_PIN_DELIVERY_STATUS		(1 << 12)
#define IOAPIC_PIN_POLARITY_ACTIVE_LOW		(1 << 13)
#define IOAPIC_PIN_TRIGGER_LEVEL		(1 << 15)
#define IOAPIC_PIN_MASKED			(1 << 16)
#define IOAPIC_PIN_DEST_FIELD_SHIFT		(56)

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

using Gsi = uint32_t;
using IrqPin = uint32_t;

class IoApic
{
private:
	volatile char *ioapic_base;
	Gsi gsi_base;
	Interrupt::InterruptVector vector_base;
public:
	IoApic(volatile char *ioapic_base, Gsi gsi_base)
		: ioapic_base(ioapic_base), gsi_base(gsi_base), vector_base(Interrupt::InvalidVector)
	{}
	~IoApic(){}

	uint32_t Read(uint32_t reg);
	void Write(uint32_t reg, uint32_t value);
	uint64_t ReadRedirectionEntry(IrqPin pin);
	void WriteRedirectionEntry(IrqPin pin, uint64_t value);
	bool AllocateInterrupts();
	void MaskPin(IrqPin pin);
	void UnmaskPin(IrqPin pin);

	inline IrqPin GetPinFromGsi(Gsi gsi) const
	{
		return gsi - gsi_base;
	}

	inline Gsi GetGsiBase() const
	{
		return gsi_base;
	}

	inline Interrupt::InterruptVector GetInterruptBase() const
	{
		return vector_base;
	}
};

class Lapic
{
private:
	volatile uint32_t *lapic_base;
	uint64_t calibration_init_tsc;
	uint64_t calibration_end_tsc;
	uint32_t ticks_in_10ms;
	unsigned long apic_rate;
	Irq::IrqHandler *handler;

	void CalibrationSetupCount();
	void CalibrationEnd();
	bool CalibrateWithAcpiTimer();
	void CalibrateWithPit();
	static bool ApicTimerIrqEntry(Irq::IrqContext& context);
public:
	Lapic(volatile uint32_t *lapic_base) : lapic_base(lapic_base) {}
	~Lapic(){}

	void Write(uint32_t reg, uint32_t value);
	uint32_t Read(uint32_t reg);

	inline void SendEoi()
	{
		Write(LAPIC_EOI, 0);
	}

	void SetupTimer();
};

constexpr unsigned int NumPins = 24;

void EarlyInit();
IoApic* GsiToApic(Gsi gsi);
Gsi MapSourceGsiToDest(Gsi source_gsi);
Gsi MapDestGsiToSrc(Gsi dest_gsi);
void Init();

}

}

#endif