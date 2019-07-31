/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>
#include <string.h>

#include <carbon/irq.h>
#include <carbon/x86/apic.h>
#include <carbon/scheduler.h>

using namespace x86;

extern "C"
unsigned long IrqEntry(uint64_t irq_number, struct registers *regs)
{
	struct Irq::IrqContext context;
	context.line = Apic::MapDestGsiToSrc(irq_number);
	context.registers = regs;
	Irq::DispatchIrq(context);

	auto thread = get_current_thread();
	if(thread->flags & THREAD_FLAG_NEEDS_RESCHED)
		context.registers = scheduler::schedule(context.registers);
	/* char buf[100];
	snprintf(buf, 100, "addr: %p\n", context.registers);
	serial_write(buf, strlen(buf), &com1);*/

	return (unsigned long) context.registers;
}