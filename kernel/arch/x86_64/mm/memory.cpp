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

bool platform_page_is_used(void *page)
{
	if((unsigned long) page == 0)
		return true;

	return false;
}