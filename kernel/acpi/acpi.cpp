/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdio.h>

#include <carbon/acpi.h>
#include <carbon/bootprotocol.h>
#include <carbon/panic.h>
#include <carbon/x86/apic.h>

#include <acpica/acpi.h>

namespace Acpi
{

void *GetRsdp()
{
	return boot_info->rsdp;
}

void Init()
{
	ACPI_STATUS st = AcpiInitializeSubsystem();
	if(ACPI_FAILURE(st))
	{
		printf("Error: %x\n", st);
		panic("ACPI subsystem initialization failed!");
	}
	st = AcpiInitializeTables(NULL, 32, true);
	if(ACPI_FAILURE(st))
	{
		printf("Error: %x\n", st);
		panic("ACPI table subsystem initialization failed!");
	}
	st = AcpiLoadTables();
	if(ACPI_FAILURE(st))
		panic("AcpiLoadTables failed!");
	
	x86::Apic::EarlyInit();

	st = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(st))
	{
		panic("AcpiEnableSubsystem failed!");
	}
	
	st = AcpiInitializeObjects (ACPI_FULL_INITIALIZATION);
	if(ACPI_FAILURE(st))
		panic("AcpiInitializeObjects failed!");
}

ACPI_TABLE_HEADER *GetTable(char *signature, UINT32 instance)
{
	ACPI_TABLE_HEADER *table = nullptr;
	
	ACPI_STATUS st = AcpiGetTable(signature, instance, &table);

	if(ACPI_FAILURE(st))
	{
		printf("Acpi: AcpiGetTable failed: error code %x\n", st);
		return nullptr;
	}

	return table;
}

bool ExecutePicMethod(InterruptModel model)
{
	ACPI_OBJECT arg;
	ACPI_OBJECT_LIST list;

	arg.Type = ACPI_TYPE_INTEGER;
	arg.Integer.Value = model;
	list.Count = 1;
	list.Pointer = &arg;

	ACPI_STATUS st = AcpiEvaluateObject(ACPI_ROOT_OBJECT, (char *)"\\_PIC", &list, NULL);

	if(ACPI_FAILURE(st))
	{
		if(st == AE_NOT_FOUND)
		{
			printf("[INFO] ExecutePicMethod: Could not find _PIC - proceeding\n");
			return true;
		}

		printf("ExecutePicMethod: AcpiEvaluateObject failed: error code %x\n", st);
		return false;
	}

	return true;
}

AcpiTimer timer;

AcpiTimer& GetTimer()
{
	return timer;
}

ClockSource::ClockTicks AcpiTimer::GetTicks()
{
	ClockTicks t = 0;
	AcpiGetTimer((UINT32*) &t);
	return t;
}

ClockSource::ClockNs AcpiTimer::GetElapsed(ClockSource::ClockTicks first, ClockSource::ClockTicks last)
{
	/* Forced to rewrite this because AcpiGetTimerDuration works with 
	 * microseconds instead of nanoseconds like we want
	*/
	uint32_t delta = 0;

	/* Convert these to uint32_t's since the timer's resolution is 32-bit max. */
	uint32_t old_ticks = (uint32_t) first;
	uint32_t new_ticks = (uint32_t) last;

	if(old_ticks < new_ticks)
	{
		delta = new_ticks - old_ticks;
	}
	else if(old_ticks == new_ticks)
	{
		return 0;
	}
	else
	{
		unsigned int res;
		AcpiGetTimerResolution(&res);
	
		if(res == 24)
		{
			delta = ((0x00ffffff - old_ticks) + new_ticks) & 0x00ffffff;
		}
		else if(res == 32)
		{
			delta = (0xffffffff - old_ticks) + new_ticks;
		}
		else
		{
			printf("acpi_pm_timer: Unknown timer resolution\n");
		}
	}
	
	unsigned int delta_time = delta * NS_PER_SEC / ACPI_PM_TIMER_FREQUENCY;
	
	return delta_time;
}

};