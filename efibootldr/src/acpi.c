/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of efibootldr, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include <stddef.h>
#include <stdint.h>

#include <carbon/bootprotocol.h>

void *acpi_get_rsdp(void)
{
	void *rsdp = NULL;
	EFI_GUID acpi_guid = ACPI_TABLE_GUID;
	if(LibGetSystemConfigurationTable(&acpi_guid, &rsdp) != EFI_SUCCESS)
		return NULL;
	
	return rsdp;
}
