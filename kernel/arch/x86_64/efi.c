/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <carbon/memory.h>
#include <carbon/bootprotocol.h>
#include <carbon/x86/serial.h>
#include <carbon/framebuffer.h>
#include <carbon/page.h>
#include <carbon/x86/idt.h>
#include <carbon/vm.h>

struct boot_info *boot_info = NULL;

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

void *__efi_allocate_early_boot_mem(size_t size)
{
	size_t desc_size = boot_info->mmap.size_descriptors;
	EFI_MEMORY_DESCRIPTOR *descriptors = boot_info->mmap.descriptors;

	for(UINTN i = 0; i < boot_info->mmap.nr_descriptors; i++, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
	{
		if(!uefi_unusable_region(descriptors))
		{
			if(descriptors->NumberOfPages << PAGE_SHIFT >= size)
			{
				descriptors->NumberOfPages -= size >> PAGE_SHIFT;
				void *ret = phys_to_virt(descriptors->PhysicalStart);
				descriptors->PhysicalStart += size;
				descriptors->VirtualStart += size;

				return ret;
			}
		}
	}
}

#define ALIGN_TO(x, y) (((unsigned long)x + (y - 1)) & -y)

void *efi_allocate_early_boot_mem(size_t size)
{
	size = ALIGN_TO(size, PAGE_SIZE);

	return __efi_allocate_early_boot_mem(size);
}

void efi_setup_physical_memory(struct boot_info *info)
{
	size_t desc_size = info->mmap.size_descriptors;
	size_t memory = 0;
	EFI_MEMORY_DESCRIPTOR *descriptors = info->mmap.descriptors;

	for(UINTN i = 0; i < info->mmap.nr_descriptors; i++, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
	{
		if(!uefi_unusable_region(descriptors))
		{
			memory += descriptors->NumberOfPages * PAGE_SIZE;
		}
	}

	page_reserve_memory(memory);

	descriptors = info->mmap.descriptors;
	for(UINTN i = 0; i < info->mmap.nr_descriptors; i++, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
	{
		if(!uefi_unusable_region(descriptors))
		{
			printf("Free region %016lx-%016lx\n", descriptors->PhysicalStart,
				descriptors->PhysicalStart +
				descriptors->NumberOfPages * PAGE_SIZE);
			page_add_region(descriptors->PhysicalStart,
				descriptors->NumberOfPages * PAGE_SIZE, info);
		}
	}
}

struct framebuffer efi_fb =
{
	.name = "efi_fb"
};

void vterm_initialize(void);

void efi_setup_framebuffer(struct boot_info *info)
{
	//printf("efifb: Framebuffer %lx\n", info->fb.framebuffer);
	efi_fb.framebuffer = phys_to_virt(info->fb.framebuffer);
	efi_fb.framebuffer_phys = info->fb.framebuffer;
	efi_fb.framebuffer_size = info->fb.framebuffer_size;
	efi_fb.width = info->fb.width;
	efi_fb.height = info->fb.height;
	efi_fb.pitch = info->fb.pitch;
	efi_fb.bpp = info->fb.bpp;
	efi_fb.color = info->fb.color;

	memset(efi_fb.framebuffer, 0, efi_fb.framebuffer_size);

	set_framebuffer(&efi_fb);

	vterm_initialize();
}

void heap_set_start(uintptr_t heap_start);

void efi_entry(struct boot_info *info)
{
	x86_serial_init();
	x86_setup_physical_mappings();

	/* Fixup boot info */
	info = phys_to_virt(info);
	info->command_line = phys_to_virt(info->command_line);
	info->mmap.descriptors = phys_to_virt(info->mmap.descriptors);
	if(info->modules) info->modules = phys_to_virt(info->modules);

	for(struct module *m = info->modules; m != NULL && m->next != NULL; m = m->next)
	{
		m->next = phys_to_virt(m->next);
	}

	boot_info = info;

	efi_setup_framebuffer(info);

	printf("carbon: Starting kernel.\n");

	efi_setup_physical_memory(info);

	x86_init_exceptions();

	uintptr_t heap_start = 0xffffa00000000000;
	heap_set_start(heap_start);

	vm_init();
	while(1)
		__asm__ __volatile__("hlt");
}
