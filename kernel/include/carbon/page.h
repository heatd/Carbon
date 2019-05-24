/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_PAGE_H
#define _CARBON_PAGE_H

#include <stdint.h>
#include <stddef.h>

#include <carbon/carbon.h>

struct page
{
	void *paddr;
	struct page *next;
	unsigned long ref;

	size_t off;		/* Offset in vmo */

	union
	{
		struct page *next_allocation;
		struct page *next_virtual_region;
	} next_un;
};


#define PAGE_ALLOC_NOZERO		(1 << 0)
#define PAGE_ALLOC_CONTIGUOUS		(1 << 1)

static inline bool page_should_zero(unsigned long flags)
{
	return !(flags & PAGE_ALLOC_NOZERO);
}

struct page *alloc_pages(size_t nr_pgs, unsigned long flags);

void free_pages(struct page *pages);
void free_page(struct page *p);

void page_reserve_memory(size_t nr);

struct boot_info;

void page_init(size_t memory_size, void *(*get_phys_mem_region)(uintptr_t *base,
	uintptr_t *size, void *context), struct boot_info *info);

void *map_phys_to_virt(uintptr_t virt, uintptr_t phys, unsigned long prot);
void *__map_phys_to_virt(HANDLE addr, uintptr_t virt, uintptr_t phys,
	unsigned long prot);

void *map_pages(void *addr, unsigned long prot, unsigned int nr_pages);

struct page *phys_to_page(uintptr_t p);
struct page *page_add_page(void *p);

static inline unsigned long page_ref(struct page *p)
{
	return __sync_add_and_fetch(&p->ref, 1);
}

static inline unsigned long page_unref(struct page *p)
{
	return __sync_sub_and_fetch(&p->ref, 1);
}

namespace Page
{

struct page_usage
{
	size_t used_pages;
	size_t total_pages;
};

void GetStats(struct page_usage *u);

}
#endif
