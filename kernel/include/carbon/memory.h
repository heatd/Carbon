/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_MEMORY_H
#define _CARBON_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#include <carbon/carbon.h>
#include <carbon/bootprotocol.h>

#define PHYS_BASE		0xffff880000000000

void x86_setup_early_physical_mappings(void);
void x86_setup_physical_mappings();
void *x86_placement_map(unsigned long phys);
void efi_setup_physical_memory(struct boot_info *info);
bool page_is_used(struct boot_info *info, void *p);

struct kernel_limits
{
	uintptr_t start_phys, start_virt;
	uintptr_t end_phys, end_virt;
};

void get_kernel_limits(struct kernel_limits *l);

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12

#define phys_to_virt(phys) 	((void *) (((uintptr_t) phys) + PHYS_BASE))

void *ksbrk(size_t size);
void *__ksbrk(long inc);
void __kbrk(void *break_);

void unmap_page_range(void *as, void *addr, size_t len);
void flush_tlb(void *addr, size_t nr_pages);

void malloc_reserve_memory_space(void);

static inline void *page_align_up(void *ptr)
{
	uintptr_t i = (uintptr_t) ptr;
	i = (i + PAGE_SIZE-1) & -PAGE_SIZE;
	return (void *) i;
}

static inline size_t size_to_pages(size_t size)
{
	size_t pages = size >> PAGE_SHIFT;
	if(size & (PAGE_SIZE-1))
		pages++;
	return pages;
}

#endif
