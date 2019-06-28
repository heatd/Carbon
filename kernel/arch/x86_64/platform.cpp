/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/x86/apic.h>
#include <carbon/platform.h>

void Platform::Irq::MaskLine(::Irq::IrqLine line)
{
	/* In x86, pin = line for APIC purposes */
	if(line < x86::Apic::NumPins)
		x86::Apic::MaskPin(line);
}

void Platform::Irq::UnmaskLine(::Irq::IrqLine line)
{
	if(line < x86::Apic::NumPins)
		x86::Apic::UnmaskPin(line);
}