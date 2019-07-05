/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdint.h>
#include <stdio.h>

#include <carbon/cpu.h>
#include <carbon/panic.h>
#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/interrupt.h>
#include <carbon/acpi.h>
#include <carbon/list.h>
#include <carbon/irq.h>
#include <carbon/scheduler.h>
#include <carbon/timer.h>
#include <carbon/percpu.h>

#include <carbon/x86/msr.h>
#include <carbon/x86/apic.h>
#include <carbon/x86/idt.h>
#include <carbon/x86/pit.h>
#include <carbon/x86/cpu.h>

namespace x86
{

namespace Apic
{

Spinlock apic_list_lock;
LinkedList<IoApic *> apic_list;

struct InterruptOverride
{
	Gsi source;
	Gsi dest;
	bool active_high;
	bool level;
};

static size_t nr_overrides = 0;
static InterruptOverride *overrides = nullptr;

uint32_t IoApic::Read(uint32_t reg)
{
	uint32_t volatile *ioapic = (uint32_t volatile*) ioapic_base;
	ioapic[0] = (reg & 0xFF);
	return ioapic[4];
}

void IoApic::Write(uint32_t reg, uint32_t value)
{
	uint32_t volatile *ioapic = (uint32_t volatile*) ioapic_base;
	ioapic[0] = (reg & 0xFF);
	ioapic[4] = value;
}

uint64_t IoApic::ReadRedirectionEntry(IrqPin pin)
{
	uint64_t ret;
	ret = (uint64_t) Read(IOAPIC_REG_REDIRECTION_BASE + pin * 2);
	ret |= (uint64_t) Read(IOAPIC_REG_REDIRECTION_BASE + pin * 2 + 1) << 32;
	return ret;
}

void IoApic::WriteRedirectionEntry(IrqPin pin, uint64_t value)
{
	Write(IOAPIC_REG_REDIRECTION_BASE + pin * 2, value & 0x00000000FFFFFFFF);
	Write(IOAPIC_REG_REDIRECTION_BASE + pin * 2 + 1, value >> 32);
}

void IoApic::UnmaskPin(IrqPin pin)
{
	/*printk("Unmasking pin %u\n", pin);*/
	uint64_t entry = ReadRedirectionEntry(pin);
	entry &= ~IOAPIC_PIN_MASKED;
	WriteRedirectionEntry(pin, entry);
}

void IoApic::MaskPin(IrqPin pin)
{
	/*printk("Masking pin %u\n", pin);*/
	uint64_t entry = ReadRedirectionEntry(pin);
	entry |= IOAPIC_PIN_MASKED;
	WriteRedirectionEntry(pin, entry);
}

bool IoApic::AllocateInterrupts()
{
	vector_base = Interrupt::AllocateInterrupts(NumPins);

	if(vector_base == Interrupt::InvalidVector)
		return false;

	unsigned long handler_size = (unsigned long) &irq1 - (unsigned long) &irq0;
	unsigned long irq0_address = (unsigned long) &irq0;
	unsigned long irqn_address = irq0_address + handler_size * gsi_base;
	/* This takes some clever math in order to not duplicate code.
	 * We essentially calculate the handlers' size and multiply in order
	 * to get the desired handler.
	*/

	for(unsigned int i = 0; i < NumPins; i++)
	{
		void (*handler)() = (void (*)())(irqn_address + i * handler_size);

		x86_reserve_vector(vector_base + i, handler);
	}

	return true;
}

void SetupIsaIrqs()
{
	IoApic *apic = GsiToApic(0);
	auto IrqBase = apic->GetInterruptBase();
 
 	for(unsigned int i = 0; i < 24; i++)
	{
		if(i <= 19)
		{
			// ISA Interrupt, set it like a standard ISA interrupt
			/*
			* ISA Interrupts have the following attributes:
			* - Active High
			* - Edge triggered
			* - Fixed delivery mode
			* They might be overwriten by the ISO descriptors in the MADT
			*/
			uint64_t entry;
			entry = IOAPIC_PIN_MASKED | (IrqBase + i);
			apic->WriteRedirectionEntry(i, entry);
		}

		uint64_t entry = apic->ReadRedirectionEntry(i);
		apic->WriteRedirectionEntry(i, entry | (IrqBase + i));
	}
}

IoApic* GsiToApic(Gsi gsi)
{
	ScopedSpinlock guard{&apic_list_lock};

	for(auto apic : apic_list)
	{
		auto gsi_base = apic->GetGsiBase();
		if(gsi >= gsi_base && gsi < gsi_base + NumPins)
			return apic;
	}

	panic("Bad gsi");

	return nullptr;
}

void ParseMadt()
{
	auto madt = Acpi::GetMadt();

	if(!madt)
	{
		panic("Acpi: Could not get MADT");
	}

	auto first_subtable = (ACPI_SUBTABLE_HEADER *) (madt + 1);
	auto last = (ACPI_TABLE_HEADER *) ((char *) madt + madt->Header.Length);

	for(auto it = first_subtable; (unsigned long) it < (unsigned long) last;
		it = (ACPI_SUBTABLE_HEADER *) ((unsigned long) it + it->Length))
	{
		if(it->Type == AcpiMadtType::ACPI_MADT_TYPE_IO_APIC)
		{
			ACPI_MADT_IO_APIC *apic = (ACPI_MADT_IO_APIC *) it;
			printf("Address: %x\n", apic->Address);
			printf("GSI base: %x\n", apic->GlobalIrqBase);
		}
		if(it->Type == AcpiMadtType::ACPI_MADT_TYPE_INTERRUPT_OVERRIDE)
		{
			ACPI_MADT_INTERRUPT_OVERRIDE *mio = (ACPI_MADT_INTERRUPT_OVERRIDE *) it;
			printf("Interrupt overide from %u to GSI %u\n", mio->SourceIrq, mio->GlobalIrq);

			bool active_high = (mio->IntiFlags & ACPI_MADT_POLARITY_MASK) == ACPI_MADT_POLARITY_ACTIVE_HIGH;
			bool level = (mio->IntiFlags & ACPI_MADT_TRIGGER_MASK) == ACPI_MADT_TRIGGER_LEVEL;

			nr_overrides++;
			InterruptOverride *new_overrides = (InterruptOverride *) realloc(overrides,
						     nr_overrides * sizeof(InterruptOverride));
			
			assert(new_overrides != nullptr);

			InterruptOverride& override = new_overrides[nr_overrides - 1];
			override.source = mio->SourceIrq;
			override.dest = mio->GlobalIrq;
			override.active_high = active_high;
			override.level = level;
			overrides = new_overrides;

			auto apic = GsiToApic(override.dest);
			auto pin = apic->GetPinFromGsi(override.dest);
 
 			uint64_t redirection_entry = IOAPIC_PIN_MASKED | (pin + apic->GetInterruptBase());

			redirection_entry |= (active_high ? 0 : IOAPIC_PIN_POLARITY_ACTIVE_LOW);
			redirection_entry |= (level ? IOAPIC_PIN_TRIGGER_LEVEL : 0);

			apic->WriteRedirectionEntry(pin, redirection_entry);
		}
	}
	
}

void Lapic::Write(uint32_t reg, uint32_t value)
{
	volatile uint32_t *laddr = (volatile uint32_t *)((volatile char*) lapic_base + reg);
	*laddr = value;
}

uint32_t Lapic::Read(uint32_t reg)
{
	volatile uint32_t *laddr = (volatile uint32_t *)((volatile char*) lapic_base + reg);
	return *laddr;
}

void Lapic::CalibrationSetupCount()
{
	Write(LAPIC_TIMER_INITCNT, 0xffffffff);

	/* Get the TSC rate as well */
	calibration_init_tsc = rdtsc();
}

void Lapic::CalibrationEnd()
{
	calibration_end_tsc = rdtsc();

	ticks_in_10ms = 0xffffffff - Read(LAPIC_TIMER_CURRCNT); 
}

void Lapic::CalibrateWithPit()
{
	printf("lapic: Using the PIT(fallback) for timer calibration\n");

	Pit::InitOneshot(100);
	
	CalibrationSetupCount();

	Pit::WaitForOneshot();

	CalibrationEnd();
}

bool Lapic::CalibrateWithAcpiTimer()
{
	UINT32 u;
	ACPI_STATUS st = AcpiGetTimer(&u);

	/* Test if the timer exists first */
	if(ACPI_FAILURE(st))
		return false;
	
	printf("lapic: Using the ACPI Timer for timer calibration\n");

	Acpi::AcpiTimer& acpi_timer = Acpi::GetTimer();

	auto start = acpi_timer.GetTicks();

	CalibrationSetupCount();

	const ClockSource::ClockNs needed_interval = 10000000;

	while(acpi_timer.GetElapsed(start, acpi_timer.GetTicks()) < needed_interval)
	{}

	CalibrationEnd();

	return true;
}

static Driver apic_driver("apic");

static Device lapic_device("lapic", &apic_driver);

PER_CPU_VAR(unsigned long apic_ticks) = 0;

bool Lapic::ApicTimerIrqEntry(Irq::IrqContext& context)
{
	add_per_cpu(apic_ticks, 1);

	Scheduler::OnTick();
	Timer::HandlePendingTimerEvents();

	return true;
}

void Lapic::SetupTimer()
{
	/* Set the timer divisor to 16 */
	Write(LAPIC_TIMER_DIV, 3);

	if(!CalibrateWithAcpiTimer())
		CalibrateWithPit();

	
	apic_rate = ticks_in_10ms / 10;

	printf("Apic rate: %lu\n", apic_rate);

	auto ioapic = GsiToApic(2);
	auto vector = ioapic->GetInterruptBase() + 2;

	Cpu::DisableInterrupts();

	Write(LAPIC_LVT_TIMER, vector | LAPIC_LVT_TIMER_MODE_PERIODIC);
	Write(LAPIC_TIMER_INITCNT, apic_rate);
	
	handler = new Irq::IrqHandler(&lapic_device, &apic_driver, ApicTimerIrqEntry, 2, this);

	Irq::InstallIrq(handler);
	Cpu::EnableInterrupts();
}

Gsi MapSourceGsiToDest(Gsi source_gsi)
{
	auto override = overrides;
	for(size_t i = 0; i < nr_overrides; i++)
	{
		if(override->source == source_gsi)
			return override->dest;
		
		override++;
	}

	return source_gsi;
}

Gsi MapDestGsiToSrc(Gsi dest_gsi)
{
#if 0
	auto override = overrides;
	for(size_t i = 0; i < nr_overrides; i++)
	{
		if(override->dest == dest_gsi)
			return override->source;
		
		override++;
	}
#endif
	return dest_gsi;
}

PER_CPU_VAR(Lapic *cpu_lapic) = nullptr;
void SetupLapic()
{
	unsigned long address = rdmsr(IA32_APIC_BASE_MSR);
	address &= ~(PAGE_SIZE - 1);

	auto mem = (volatile uint32_t *) Vm::MmioMap(&kernel_address_space,
					address, 0, PAGE_SIZE, VM_PROT_WRITE);

	assert(mem != nullptr);

	Lapic *this_lapic = new Lapic(mem);

	assert(this_lapic != nullptr);

	/* Enable the LAPIC by setting LAPIC_SPUINT to 0x100 OR'd with the default spurious IRQ(15) */

	this_lapic->Write(LAPIC_SPUINT, 0x100 | APIC_DEFAULT_SPURIOUS_IRQ);
	
	/* Send an EOI to unstuck pending interrupts */
	this_lapic->SendEoi();

	this_lapic->Write(LAPIC_TSKPRI, 0);

	write_per_cpu(cpu_lapic, this_lapic);

	this_lapic->SetupTimer();
}

void EarlyInit()
{
	auto ioapic_base = (volatile char *) Vm::MmioMap(&kernel_address_space,
			IOAPIC_BASE_PHYS, 0, PAGE_SIZE, VM_PROT_WRITE);

	assert(ioapic_base != nullptr);

	auto apic = new IoApic(ioapic_base, 0);

	assert(apic != nullptr);

	assert(apic->AllocateInterrupts() == true);

	assert(apic_list.Add(apic) == true);

	SetupIsaIrqs();

	ParseMadt();

	SetupLapic();
}

void Init()
{
	assert(Acpi::ExecutePicMethod(Acpi::InterruptModel::Apic) == true);
}


/* Stubs for the IRQ code */
extern "C"
void platform_send_eoi()
{
	get_per_cpu(cpu_lapic)->SendEoi();
}

extern "C"
bool signal_is_pending()
{
	return false;
}

extern "C"
void handle_signal()
{}

}
}