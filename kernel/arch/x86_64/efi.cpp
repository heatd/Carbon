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
#include <limits.h>

#include <carbon/percpu.h>
#include <carbon/page.h>
#include <carbon/memory.h>
#include <carbon/bootprotocol.h>
#include <carbon/x86/serial.h>
#include <carbon/framebuffer.h>
#include <carbon/page.h>
#include <carbon/x86/idt.h>
#include <carbon/vm.h>
#include <carbon/x86/cpu.h>
#include <carbon/x86/apic.h>
#include <carbon/acpi.h>
#include <carbon/list.h>
#include <carbon/scheduler.h>
#include <carbon/cpu.h>
#include <carbon/smp.h>

#include <carbon/fpu.h>
#include <carbon/x86/gdt.h>

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
		return false;
	if(desc->Type == EfiBootServicesData)
		return false;
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
	if(desc->Type == EfiPalCode)
		return true;
	return false;
}

#define ALIGN_TO(x, y) (((unsigned long)x + (y - 1)) & -y)

bool check_for_contig(EFI_MEMORY_DESCRIPTOR *descriptor, size_t size)
{
	for(size_t j = 0; j < size_to_pages(size); j++)
	{
		if(page_is_used(boot_info,
		(void *) (descriptor->PhysicalStart + j * PAGE_SIZE)) == true)
			return false;
	}

	return true;
}

bool check_kernel_limits(void *__page);

bool physical_mem_inited = false;

static inline void *temp_map_mem(unsigned long mem)
{
	if(physical_mem_inited)
		return (void *) mem;
	else
		return x86_placement_map(mem);
}

void *__efi_allocate_early_boot_mem(size_t size)
{
	struct boot_info *binfo = (struct boot_info *) temp_map_mem((unsigned long) boot_info);
	size_t desc_size = binfo->mmap.size_descriptors;
	size_t nr_descs = binfo->mmap.nr_descriptors;
	EFI_MEMORY_DESCRIPTOR *__descriptors = binfo->mmap.descriptors;

	for(UINTN i = 0; i < nr_descs; i++, __descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) __descriptors + desc_size))
	{
		EFI_MEMORY_DESCRIPTOR *descriptors =
			(EFI_MEMORY_DESCRIPTOR *) temp_map_mem((unsigned long) __descriptors);

		/* Only use safe memory types since then we don't have to check for modules, etc */
		if(descriptors->Type == EfiConventionalMemory || descriptors->Type == EfiBootServicesData ||
			descriptors->Type == EfiBootServicesCode)
		{
			if(descriptors->PhysicalStart == 0)
			{
				/* NULL page might be interpreted as an OOM, so skip it */
				descriptors->PhysicalStart += PAGE_SIZE;
				descriptors->NumberOfPages--;
			}

			if(descriptors->NumberOfPages << PAGE_SHIFT >= size)
			{
				/* Note: We only check at the beginning of
				  descriptors for simplicity, since otherwise
				  we would need to split them bcs of holes */
				if(check_kernel_limits((void *) descriptors->PhysicalStart) == true)
					continue;
				descriptors->NumberOfPages -= size >> PAGE_SHIFT;
				void *ret = (void *) descriptors->PhysicalStart;
				descriptors->PhysicalStart += size;
				descriptors->VirtualStart += size;

				return ret;
			}
		}
	}

	return NULL;
}

void *efi_allocate_early_boot_mem(size_t size)
{
	size = ALIGN_TO(size, PAGE_SIZE);

	return __efi_allocate_early_boot_mem(size);
}

static size_t desc_size;
static size_t nr_descs;
static EFI_MEMORY_DESCRIPTOR *gbl_descriptors;

void *efi_get_phys_mem_reg(uintptr_t *base, uintptr_t *size, void *context)
{
	EFI_MEMORY_DESCRIPTOR *descriptors = gbl_descriptors;

	size_t curr_entry = (size_t) context;

	if(curr_entry == 0)
	{
		/* If we're getting started, look for the first usable region */

		for(; curr_entry != nr_descs; ++curr_entry, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
		{
			if(!uefi_unusable_region(descriptors))
				break;
		}
	}

	if(curr_entry == nr_descs)
		return NULL;

	for(UINTN i = 0; i < curr_entry; i++, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size));
	printf("Free region at %lu %016lx-%016lx\n", curr_entry, descriptors->PhysicalStart,
				descriptors->PhysicalStart +
				descriptors->NumberOfPages * PAGE_SIZE);

	*base = descriptors->PhysicalStart;
	*size = descriptors->NumberOfPages * PAGE_SIZE;

	curr_entry++;
	descriptors = (EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size);

	for(; curr_entry != nr_descs; ++curr_entry, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
	{
		if(!uefi_unusable_region(descriptors))
			break;
	}

	return (void *) curr_entry;

}

void efi_setup_physical_memory(struct boot_info *info)
{
	desc_size = info->mmap.size_descriptors;
	nr_descs = info->mmap.nr_descriptors;

	size_t memory = 0;
	EFI_MEMORY_DESCRIPTOR *descriptors = gbl_descriptors = info->mmap.descriptors;

	for(UINTN i = 0; i < info->mmap.nr_descriptors; i++, descriptors =
		(EFI_MEMORY_DESCRIPTOR*) ((uintptr_t) descriptors + desc_size))
	{
		if(!uefi_unusable_region(descriptors))
		{
			memory += descriptors->NumberOfPages * PAGE_SIZE;
		}
	}

	page_init(memory, efi_get_phys_mem_reg, info);
}

struct framebuffer efi_fb =
{
	.name = "efi_fb",
	0,
	nullptr,
	0, 0, 0, 0, 0,
	{}
};

void vterm_initialize(void);

void setup_debug_register(unsigned long addr, unsigned int size, unsigned int condition)
{
	unsigned long dr7 = 1 | size << 18 | condition << 16;

	__asm__ volatile("mov %0, %%dr0; mov %1, %%dr7" :: "r"(addr), "r"(dr7));
}

void asan_init();
extern "C" void _init();

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

void thread_test(void *context)
{
	while(true)
	{
		Scheduler::Block(get_current_thread());
	}
}

void paging_protect_kernel(void);

extern "C" uint64_t bsp_gdt;
extern "C" void efi_entry(struct boot_info *info)
{
	x86_serial_init();
	x86_setup_early_physical_mappings();

	boot_info = info;

	x86::Cpu::Identify();

	x86_setup_physical_mappings();

	/* Fixup boot info */
	boot_info = info = (struct boot_info *) phys_to_virt(info);
	info->command_line = (wchar_t *) phys_to_virt(info->command_line);
	info->mmap.descriptors = (EFI_MEMORY_DESCRIPTOR *) phys_to_virt(info->mmap.descriptors);
	if(info->modules) info->modules = (struct module *) phys_to_virt(info->modules);

	for(struct module *m = info->modules; m != NULL && m->next != NULL; m = m->next)
	{
		m->next = (struct module *) phys_to_virt(m->next);
	}

	physical_mem_inited = true;

	efi_setup_framebuffer(info);

	printf("carbon: Starting kernel.\n");

	efi_setup_physical_memory(info);
	
	paging_protect_kernel();


	x86_init_exceptions();

	uintptr_t heap_start = 0xffffa00000000000;
	heap_set_start(heap_start);

	vm_init();

#if 0
	asan_init();
#endif

	/* Invoke global constructors */
	_init();

	Percpu::Init();

	Gdt::InitPercpu();

	Fpu::Init();

	Scheduler::Initialize();

	Acpi::Init();

	x86::Apic::Init();

	Smp::BootCpus();

	Scheduler::Block(get_current_thread());
}
