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
		printf("Error: %lx\n", st);
		panic("ACPI subsystem initialization failed!");
	}
	st = AcpiInitializeTables(NULL, 32, true);
	if(ACPI_FAILURE(st))
	{
		printf("Error: %lx\n", st);
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

};