/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_ACPI_H
#define _CARBON_ACPI_H

#include <acpica/acpi.h>

#include <carbon/clocksource.h>

namespace Acpi
{

void *GetRsdp();
void Init();

ACPI_TABLE_HEADER *GetTable(char *signature, UINT32 instance = 0);

inline ACPI_TABLE_MADT *GetMadt()
{
	return reinterpret_cast<ACPI_TABLE_MADT *>(GetTable(ACPI_SIG_MADT));
}

enum InterruptModel
{
	Pic = 0,
	Apic = 1,
	Sapic = 2
};

bool ExecutePicMethod(InterruptModel model);

class AcpiTimer : public ClockSource
{
public:
	AcpiTimer() : ClockSource("AcpiTimer", ACPI_PM_TIMER_FREQUENCY, 200) {}
	~AcpiTimer() {}

	ClockTicks GetTicks() override;

	ClockNs GetElapsed(ClockTicks first, ClockTicks last) override;
};

AcpiTimer& GetTimer();

};

#endif