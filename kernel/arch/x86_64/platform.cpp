/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/x86/apic.h>
#include <carbon/platform.h>

/* Eww */
using namespace x86;

void Platform::Irq::MaskLine(::Irq::IrqLine gsi)
{
	/* In x86, pin = line for APIC purposes */
	if(!(gsi < x86::Apic::NumPins))
		return;

	gsi = Apic::MapSourceGsiToDest(gsi);

	Apic::IoApic *apic = Apic::GsiToApic(gsi);	

	Apic::IrqPin pin = apic->GetPinFromGsi(gsi);

	apic->MaskPin(pin);
}

void Platform::Irq::UnmaskLine(::Irq::IrqLine gsi)
{
	if(!(gsi < x86::Apic::NumPins))
		return;

	gsi = Apic::MapSourceGsiToDest(gsi);
	Apic::IoApic *apic = Apic::GsiToApic(gsi);	

	Apic::IrqPin pin = apic->GetPinFromGsi(gsi);

	apic->UnmaskPin(pin);

}