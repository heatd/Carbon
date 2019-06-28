/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_IRQ_H
#define _CARBON_IRQ_H

#include <carbon/device.h>

#define NR_IRQ 				221

namespace Irq
{

class IrqHandler;
bool InstallIrq(IrqHandler *handler);
void FreeIrq(IrqHandler *handler);

using IrqLine = unsigned int;

#define IRQ_BAD_LINE		((unsigned int) -1)

struct registers;
struct IrqContext
{
	struct registers *registers;
	Device *device;
	Device *driver;
	IrqLine line;
	void *context;
};

using IrqCallback = bool (*)(struct IrqContext& context);

class IrqHandler
{
private:
	IrqLine line;
	Device *device;
	Driver *driver;
	IrqCallback callback;
	void *context;
public:
	IrqHandler(Device *device, Driver *driver, IrqCallback cb, IrqLine line, void *context = nullptr)
		   :  line(line), device(device), driver(driver), callback(cb), context(context) {}
	~IrqHandler() {}

	bool Install()
	{
		return InstallIrq(this);
	}

	void Free()
	{
		return FreeIrq(this);
	}

	inline IrqLine GetLine()
	{
		return line;
	}

	inline IrqCallback GetCallback()
	{
		return callback;
	}

	inline void SetContext(void *Context)
	{
		context = Context;
	}

	inline void *GetContext()
	{
		return context;
	}
};

}

#endif