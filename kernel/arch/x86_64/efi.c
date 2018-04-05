/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <carbon/memory.h>
#include <carbon/bootprotocol.h>
#include <carbon/x86/serial.h>

bool uefi_unusable_region(EFI_MEMORY_DESCRIPTOR *desc)
{
	/* We can't use EfiLoaderData regions as they're important 
	 * Maybe we can use another memory desc type in the bootloader 
	 * for more efficient memory usage
	*/
	if(desc->Type == EfiLoaderData)
		return true;
	/* Get rid of the boot services' code and data */
	if(desc->Type == EfiBootServicesCode)
		return true;
	if(desc->Type == EfiBootServicesData)
		return true;
	/* We can't use EfiUnusableMemory memory descs because I don't fucking know, 
	 * UEFI marks some random areas as unusable memory
	 * Anyway, don't use this memory unless we want to brick firmware or some shit
	*/
	if(desc->Type == EfiUnusableMemory)
		return true;
	if(desc->Type == EfiReservedMemoryType)
		return true;
	/* Memory mapped I/O is also unusable */
	if(desc->Type == EfiMemoryMappedIO)
		return true;
	if(desc->Type == EfiMemoryMappedIOPortSpace)
		return true;
	/* We also can't use ACPI memory */
	if(desc->Type == EfiACPIReclaimMemory)
		return true;
	if(desc->Type == EfiACPIMemoryNVS)
		return true;
	/* Lets not overwrite runtime services memory, shall we?
	 * We'll need this memory to call the runtime services if the user wants
	 * to i.e set an NVRAM variable
	*/
	if(desc->Type == EfiRuntimeServicesCode)
		return true;
	if(desc->Type == EfiRuntimeServicesData)
		return true;
	/* TODO: EfiMaxMemoryType and EfiPalCode */
	return false;
}

void efi_setup_physical_memory(struct boot_info *info)
{
	size_t desc_size = info->mmap.size_descriptors;
	size_t memory = 0;
	EFI_MEMORY_DESCRIPTOR *descriptors = info->mmap.descriptors;
	for(UINTN i = 0; i < info->mmap.nr_descriptors; i++, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
	{
		if((descriptors->PhysicalStart + descriptors->NumberOfPages * PAGE_SIZE) > memory)
			memory = descriptors->PhysicalStart + descriptors->NumberOfPages * PAGE_SIZE;
		printf("Desc: %p\n", descriptors);
		printf("Region %016lx-%016lx\n", descriptors->PhysicalStart,
				descriptors->PhysicalStart +
				descriptors->NumberOfPages * PAGE_SIZE);
		printf("Type: %x\n", descriptors->Type);
		if(!uefi_unusable_region(descriptors))
		{
			page_add_region(descriptors->PhysicalStart,
				descriptors->NumberOfPages * PAGE_SIZE, info);
		}
	}
	
}

void efi_entry(struct boot_info *info)
{
	x86_setup_physical_mappings();
	x86_serial_init();

	/* Fixup boot info */
	info = phys_to_virt(info);
	info->command_line = phys_to_virt(info->command_line);
	info->mmap.descriptors = phys_to_virt(info->mmap.descriptors);
	info->modules = phys_to_virt(info->modules);

	for(struct module *m = info->modules; m != NULL && m->next != NULL; m = m->next)
	{
		m->next = phys_to_virt(m->next);
	}

	efi_setup_physical_memory(info);

}
