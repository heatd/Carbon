/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/irq.h>
#include <carbon/platform.h>
#include <carbon/lock.h>
namespace Irq
{

LinkedList<IrqHandler *> irq_lines[NR_IRQ];
static Spinlock irq_lock;

bool InstallIrq(IrqHandler *handler)
{
	ScopedSpinlock l(&irq_lock);
	IrqLine line = handler->GetLine();

	assert(line < NR_IRQ);

	bool st = irq_lines[line].Add(handler);

	if(!st)
		return false;
	
	Platform::Irq::UnmaskLine(line);

	return true;
}

void FreeIrq(IrqHandler *handler)
{
	ScopedSpinlock l(&irq_lock);

	auto line = handler->GetLine();
	auto& list = irq_lines[line];
	bool st = list.Remove(handler);

	assert(st != false);

	if(irq_lines[line].IsEmpty())
	{
		Platform::Irq::MaskLine(line);
	}
}

void DispatchIrq(IrqContext& context)
{
	auto line = context.line;

	auto& list = irq_lines[line];

	for(auto handler : list)
	{
		context.device = handler->GetDevice();
		context.driver = handler->GetDriver();
		context.context = handler->GetContext();

		auto callback = handler->GetCallback();

		if(callback(context) == true)
		{
			break;
		}
	}
}

}