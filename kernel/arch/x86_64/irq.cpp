/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/irq.h>
#include <carbon/x86/apic.h>

using namespace x86;
extern "C"
unsigned long IrqEntry(uint64_t irq_number, struct registers *regs)
{
	struct Irq::IrqContext context;
	context.line = Apic::MapDestGsiToSrc(irq_number);
	context.registers = regs;
	Irq::DispatchIrq(context);

	return (unsigned long) context.registers;
}