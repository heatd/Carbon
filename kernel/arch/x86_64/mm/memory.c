/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>

#include <carbon/memory.h>

extern char kernel_start[0];
extern char kernel_end[0];

extern uintptr_t base_address;


uintptr_t get_kernel_base_address(void)
{
	return base_address;
}

void get_kernel_limits(struct kernel_limits *l)
{
	uintptr_t start_virt = (uintptr_t) &kernel_start;
	uintptr_t end_virt = (uintptr_t) &kernel_end;

	uintptr_t kernel_base = get_kernel_base_address();

	/* Note: Because the relocations to kernel_start and kernel_end are
	 * patched, they will always be virtual addresses. This normally would
	 * not happen if there were no relocations.
	*/

	l->start_virt = start_virt;
	l->end_virt = end_virt;

	l->start_phys = start_virt - kernel_base;
	l->end_phys = end_virt - kernel_base;
}

bool klimits_present = false;

bool page_is_used(struct boot_info *info, void *p)
{
	uintptr_t page = (uintptr_t) p;

	/* First check if it's between the kernel's addresses */
	static struct kernel_limits l;
	
	if(!klimits_present)
	{
		klimits_present = true;
		get_kernel_limits(&l);
		printf("Kernel limits: %lx-%lx phys, %lx-%lx virt\n", l.start_phys,
		l.end_phys, l.start_virt, l.end_virt);
	}

	uintptr_t kernel_brk = (uintptr_t) ksbrk(0);

	kernel_brk -= get_kernel_base_address();

	if(page >= l.start_phys && page < kernel_brk)
	{
		/* Between the kernel and the data segment, is used */
		return true;
	}

	/* Now check if the page is used by any modules */
	for(struct module *m = info->modules; m != NULL; m = m->next)
	{
		uintptr_t module_end = m->start + m->size;
		if(page >= m->start && page < module_end)
			return true;
	}

	return false;
}
