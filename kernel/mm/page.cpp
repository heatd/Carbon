/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <carbon/page.h>
#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/bootprotocol.h>
#include <carbon/fnv.h>

#define PAGE_HASHTABLE_ENTRIES 0x4000

struct page_hashtable
{
	struct page *table[PAGE_HASHTABLE_ENTRIES];
	struct page *tail[PAGE_HASHTABLE_ENTRIES];
};

static struct page_hashtable hashtable = {0};
static size_t num_pages = 0;

unsigned int page_hash(uintptr_t p)
{
	unsigned int hash = fnv_hash(&p, sizeof(uintptr_t));
	return hash % PAGE_HASHTABLE_ENTRIES;
}

static void append_to_hash(unsigned int hash, struct page *page)
{
	if(!hashtable.table[hash])
	{
		hashtable.table[hash] = hashtable.tail[hash] = page;
	}
	else
	{
		hashtable.tail[hash]->next = page;
		hashtable.tail[hash] = page;
	}
}

struct page *page_add_page(void *paddr)
{
	static size_t counter = 0;
	counter++;
	unsigned int hash = page_hash((uintptr_t) paddr);
	struct page *page = (struct page *) __ksbrk(sizeof(struct page));

	assert(page != NULL);

	memset(page, 0, sizeof(*page));

	page->paddr = paddr;
	page->ref = 0;
	page->next = NULL;
	page->next_un.next_allocation = NULL;
	append_to_hash(hash, page);
	++num_pages;

	return page;
}

void page_add_page_late(void *paddr)
{
	static size_t counter = 0;
	counter++;
	unsigned int hash = page_hash((uintptr_t) paddr);
	struct page *page = (struct page *) zalloc(sizeof(struct page));

	assert(page != NULL);

	page->paddr = paddr;
	page->ref = 0;
	page->next = NULL;
	append_to_hash(hash, page);
	++num_pages;
}

struct page *phys_to_page(uintptr_t phys)
{
	unsigned int hash = page_hash(phys);
	struct page *p = hashtable.table[hash];
	for(; p; p = p->next)
	{
		if(p->paddr == (void*) phys)
			return p;
	}

	printf("page: %p queried for %lx, but it doesn't exist!\n", __builtin_return_address(0), phys);
	return NULL;
}


bool klimits_present = false;

uintptr_t get_kernel_base_address(void);

bool check_kernel_limits(void *__page)
{
	static struct kernel_limits l;
	uintptr_t page = (uintptr_t) __page;

	if(!klimits_present)
	{
		klimits_present = true;
		get_kernel_limits(&l);
		printf("Kernel limits: %lx-%lx phys, %lx-%lx virt\n", l.start_phys,
		l.end_phys, l.start_virt, l.end_virt);
	}

	if(page >= l.start_phys && page < ((uintptr_t) ksbrk(0) - get_kernel_base_address()))
		return true;
	
	return false;
}

struct used_pages *used_pages_list = NULL;

struct used_pages
{
	uintptr_t start;
	uintptr_t end;
	struct used_pages *next;
};

void page_add_used_pages(struct used_pages *pages)
{
	struct used_pages **pp = &used_pages_list;

	while(*pp != NULL)
		pp = &(*pp)->next;

	*pp = pages;	
}

bool platform_page_is_used(void *page);

bool page_is_used(struct boot_info *info, void *__page)
{
	uintptr_t page = (uintptr_t) __page;

	for(struct module *m = info->modules; m != NULL; m = m->next)
	{
		if(page >= m->start && m->start + m->size > page)
			return true;
	}

	for(struct used_pages *p = used_pages_list; p; p = p->next)
	{
		if(page >= p->start && p->end > page)
			return true;
	}

	if(check_kernel_limits(__page) == true)
		return true;

	return platform_page_is_used(__page);
}

void page_print_shared(void)
{
	for(unsigned int i = 0; i < PAGE_HASHTABLE_ENTRIES; i++)
	{
		for(struct page *p = hashtable.table[i]; p != NULL; p = p->next)
		{
			if(p->ref != 1 && p->ref != 0)
				printf("Page %p has ref %lu\n", p->paddr, p->ref);
		}
	}
}