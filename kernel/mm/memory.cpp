/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stddef.h>

#include <carbon/memory.h>
#include <carbon/page.h>

extern unsigned char kernel_end;

static unsigned char *kernel_break = &kernel_end;

void *ksbrk(size_t size)
{
	/* Align the size to a multiple of 16 */
	size = (size + 16) & -16;
	unsigned char *ret = kernel_break;
	kernel_break += size;
	return ret;
}

void *map_pages(void *addr, unsigned long prot, unsigned int nr_pages)
{
	unsigned int og_pages = nr_pages;
	uintptr_t _a = (uintptr_t) addr;
	while(nr_pages--)
	{
		struct page *p = alloc_pages(1, 0);
		if(!p)
			return NULL;

		if(!map_phys_to_virt(_a, (uintptr_t) p->paddr, prot))
		{
			free_page(p);
			return NULL;
		}
		_a += PAGE_SIZE;
	}

	flush_tlb(addr, og_pages);
	return addr;
}
