/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of efibootldr, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>
#include <stddef.h>
#include <stdint.h>

struct module
{
	char name[256];
	uint64_t start;
	uint64_t size;
	struct modules *next;
};

void append_module(struct module *module);
int load_initrd(EFI_FILE_PROTOCOL *root, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS st;
	EFI_FILE_PROTOCOL *initrd;
	EFI_GUID file_info = EFI_FILE_INFO_ID;
	void *buffer;
	UINTN size;
	EFI_FILE_INFO *info;
	if((st = root->Open(root, &initrd, L"initrd.tar", EFI_FILE_MODE_READ, 0)) != EFI_SUCCESS)
	{
		Print(L"Error: Open(): Could not open initrd - error code 0x%x\n", st);
		return -1;
	}
	if((st = SystemTable->BootServices->AllocatePool(EfiLoaderData,
		sizeof(EFI_FILE_INFO), (void**) &info)) != EFI_SUCCESS)
	{
		Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
		return -1;
	}
	size = sizeof(EFI_FILE_INFO);
retry:
	if((st = initrd->GetInfo(initrd, &file_info, &size, (void*) info)) != EFI_SUCCESS)
	{
		if(st == EFI_BUFFER_TOO_SMALL)
		{
			SystemTable->BootServices->FreePool(info);
			if((st = SystemTable->BootServices->AllocatePool(
				EfiLoaderData, size, (void**) &info)) != EFI_SUCCESS)
			{
				Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
				return -1;
			}
			goto retry;
		}
		Print(L"Error: GetInfo: Could not get file info - error code 0x%x\n", st);
		return -1;
	}
	size = info->FileSize;
	UINTN pages = size % 4096 ? (size / 4096) + 1 : size / 4096;
	if((st = SystemTable->BootServices->AllocatePages(AllocateAnyPages, 
		EfiLoaderData, pages, (void**) &buffer)) != EFI_SUCCESS)
	{
		SystemTable->BootServices->FreePool(info);
		Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
		return -1;
	}

	if((st = initrd->Read(initrd, &size, buffer)) != EFI_SUCCESS)
	{
		Print(L"Error: Read: Could not read the file - error code 0x%x\n", st);
		SystemTable->BootServices->FreePool(info);
		SystemTable->BootServices->FreePages(buffer, pages);
		return -1;
	}
	struct module *module;
	if((st = SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(struct module), &module)) != EFI_SUCCESS)
	{
		Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
		return -1;
	}
	module->next = NULL;
	StrCpy(module->name, "Initrd");
	module->start = buffer;
	module->size = size;
	append_module(module);
	return 0;
}
