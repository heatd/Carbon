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

#include <carbon/bootprotocol.h>
#include <carbon/memory.h>
#include <carbon/lock.h>

#define min(t1, t2) (t1 < t2 ? t1 : t2)

size_t avail_pool = 0;
unsigned char *pool = NULL;

static inline unsigned long pow2(int exp)
{
	return (1UL << (unsigned long) exp);
}

struct page_list 
{
	struct page_list *prev;
	struct page_list *next;
};

struct page_cpu
{
	struct page_arena *arenas;
	struct processor *cpu;
	struct page_cpu *next;
};

struct page_arena
{
	unsigned long free_pages;
	unsigned long nr_pages;
	void *start_arena;
	void *end_arena;
	struct page_list *page_list;
	struct spinlock lock;
	struct page_arena *next;
};

struct page_cpu main_cpu = {0};

#define for_every_arena(cpu)	for(struct page_arena *arena = (cpu)->arenas; arena; \
	arena = arena->next)


void *page_alloc_from_arena(size_t nr_pages, unsigned long flags, struct page_arena *arena)
{
	struct page_list *p = arena->page_list;
	size_t found_pages = 0;
	uintptr_t base = 0;
	struct page_list *base_pg = NULL;
	bool found_base = false;

	spin_lock(&arena->lock);
	if(arena->free_pages < nr_pages)
	{
		spin_unlock(&arena->lock);
		return NULL;
	}

	/* Look for contiguous pages */
	for(; p && found_pages != nr_pages; p = p->next)
	{
		if((uintptr_t) p->next - (uintptr_t) p > PAGE_SIZE && nr_pages != 1)
		{
			found_pages = 0;
			found_base = false;
			break;
		}
		else
		{
			if(found_base == false)
			{
				base = (uintptr_t) p;
				found_base = true;
			}
			++found_pages;
		}
	}

	/* If we haven't found nr_pages contiguous pages, continue the search */
	if(found_pages != nr_pages)
	{
		spin_unlock(&arena->lock);
		return NULL;
	}
	else
	{
		base_pg = (struct page_list *) base;
		struct page_list *head = base_pg->prev;
		struct page_list *tail = base_pg;

		arena->free_pages -= found_pages;

		while(found_pages--)
			tail = tail->next;

		if(head)
			head->next = tail;
		else
			arena->page_list = tail;

		if(tail)
			tail->prev = head;

		spin_unlock(&arena->lock);

		return (void*) base - PHYS_BASE;
	}
}

void *page_alloc(size_t nr_pages, unsigned long flags)
{
	void *pages = NULL;
	for_every_arena(&main_cpu)
	{
		if(arena->free_pages == 0)
			continue;
		if((pages = page_alloc_from_arena(nr_pages, flags, arena)) != NULL)
		{
			return pages;
		}
	}

	return NULL;
}

void page_free_pages(struct page_arena *arena, void *addr, size_t nr_pages)
{
	spin_lock(&arena->lock);

	if(!arena->page_list)
	{
		struct page_list *list = NULL;
		uintptr_t b = (uintptr_t) addr;
		for(size_t i = 0; i < nr_pages; i++, b += PAGE_SIZE)
		{
			struct page_list *l = phys_to_virt(b);
			l->next = NULL;
			if(!list)
			{
				arena->page_list = l;
				l->prev = NULL;
				list = l;	
			}
			else
			{
				list->next = l;
				l->prev = list;
				list = l;
			}

		}
	}
	else
	{
		struct page_list *list = arena->page_list;
		while(list->next) list = list->next;
		
		uintptr_t b = (uintptr_t) addr;
		for(size_t i = 0; i < nr_pages; i++, b += PAGE_SIZE)
		{
			struct page_list *l = phys_to_virt(b);
			l->next = NULL;
			list->next = l;
			l->prev = list;
			list = l;
		}
	}

	spin_unlock(&arena->lock);
}

void page_free(size_t nr_pages, void *addr)
{
	for_every_arena(&main_cpu)
	{
		if((uintptr_t) arena->start_arena <= (uintptr_t) addr && 
			(uintptr_t) arena->end_arena > (uintptr_t) addr)
		{
			page_free_pages(arena, addr, nr_pages);
		}
	}
}

static void page_add(struct page_list **list, void *__page,
	struct boot_info *info, struct page_arena *arena)
{
	if(page_is_used(info, __page))
	{
		arena->free_pages--;
		return;
	}

	struct page_list *page = phys_to_virt(__page);
	page->next = NULL;

	if(*list)
	{
		while(*list)
			list = &(*list)->next;
		page->prev = (struct page_list *) ((char*) list - 
			offsetof(struct page_list, next));
	}
	else
		page->prev = NULL;
	*list = page;
}

static void append_arena(struct page_cpu *cpu, struct page_arena *arena)
{
	struct page_arena **a = &cpu->arenas;

	while(*a)
		a = &(*a)->next;
	*a = arena;
}

static void *alloc_pool(size_t size)
{
	void *ret = pool;
	pool += size;
	return ret;
}

void page_add_region(uintptr_t base, size_t size, struct boot_info *info)
{
	while(size)
	{
		size_t area_size = min(size, 0x200000);
		struct page_arena *arena = alloc_pool(sizeof(struct page_arena));
		assert(arena != NULL);
		memset_s(arena, 0, sizeof(struct page_arena));

		arena->free_pages = arena->nr_pages = area_size >> PAGE_SHIFT;
		arena->start_arena = (void*) base;
		arena->end_arena = (void*) (base + area_size);

		for(size_t i = 0; i < area_size; i += PAGE_SIZE)
		{
			page_add(&arena->page_list, (void*) (base + i), info, arena);
		}

		append_arena(&main_cpu, arena);

		size -= area_size;
		base += area_size;
	}
}

void *__alloc_pages_nozero(int order)
{
	size_t nr_pages = pow2(order);

	void *p = page_alloc(nr_pages, 0);
	
	return p;
}

void *__alloc_pages(int order)
{
	void *p = __alloc_pages_nozero(order);

	size_t nr_pages = pow2(order);

	if(p)
	{
		memset(phys_to_virt(p), 0, nr_pages << PAGE_SHIFT);
	}
	return p;
}

void *__alloc_page(int opt)
{
	return __alloc_pages(0);
}

void __free_pages(void *pages, int order)
{
	page_free(pow2(order), pages);
}

void __free_page(void *page)
{
	__free_pages(page, 0);
}

void *efi_allocate_early_boot_mem(size_t size);

void page_reserve_memory(size_t memory)
{
	avail_pool = (memory / 0x200000) * sizeof(struct page_arena);
	pool = efi_allocate_early_boot_mem(avail_pool);
}

#if 0
struct page *get_phys_pages(int order)
{
	void *addr = __alloc_pages(order);
	
	size_t nr_pages = pow2(order);
	if(!addr)
		return NULL;

	uintptr_t paddr = (uintptr_t) addr;

	struct page *ret = phys_to_page(paddr);

	for(; nr_pages; nr_pages--)
	{
		page_increment_refcount((void*) paddr);
	}
	return ret;
}

struct page *get_phys_page(void)
{
	void *addr = __alloc_page(0);

	if(!addr)
		return NULL;

	struct page *p = phys_to_page((uintptr_t) addr);
	p->ref++;
	return p;
}

#endif

