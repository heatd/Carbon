/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <carbon/carbon.h>
#include <carbon/lock.h>
#include <carbon/page.h>
#include <carbon/memory.h>
#include <carbon/vm.h>
#include <carbon/panic.h>

size_t page_memory_size;
size_t nr_global_pages;
static size_t used_pages = 0;

#define min(t1, t2) (t1 < t2 ? t1 : t2)

static inline unsigned long pow2(int exp)
{
	return (1UL << (unsigned long) exp);
}

struct page_list 
{
	struct page_list *prev;
	struct page *page;
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
	struct page_list *tail;
	Spinlock lock;
	struct page_arena *next;
};

static bool page_is_initialized = false;

struct page_cpu main_cpu = {};

#define for_every_arena(cpu)	for(struct page_arena *arena = (cpu)->arenas; arena; \
	arena = arena->next)


struct page *page_alloc_from_arena(size_t nr_pages, unsigned long flags, struct page_arena *arena)
{
	struct page_list *p = arena->page_list;
	size_t found_pages = 0;
	uintptr_t base = 0;
	struct page_list *base_pg = NULL;
	bool found_base = false;

	ScopedSpinlock lock(&arena->lock);

	if(arena->free_pages < nr_pages)
	{
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

		lock.Unlock();

		struct page *plist = NULL;
		struct page_list *pl = base_pg;
	
		for(size_t i = 0; i < nr_pages; i++)
		{
			assert(pl->page->ref == 0);
			page_ref(pl->page);

			if(!plist)
			{
				plist = pl->page;
			}
			else
			{
				plist->next_un.next_allocation = pl->page;
				plist = pl->page;
			}

			pl = pl->next;
		}

		return base_pg->page;
	}
}

struct page *page_alloc(size_t nr_pages, unsigned long flags)
{
	struct page *pages = NULL;
	for_every_arena(&main_cpu)
	{
		if(arena->free_pages == 0)
			continue;
		if((pages = page_alloc_from_arena(nr_pages, flags, arena)) != NULL)
		{
			__sync_add_and_fetch(&used_pages, nr_pages);
			return pages;
		}
	}

	return NULL;
}

static void append_page(struct page_arena *arena, struct page_list *page)
{
	if(!arena->page_list)
	{
		arena->page_list = arena->tail = page;
		page->next = NULL;
		page->prev = NULL;
	}
	else
	{
		arena->tail->next = page;
		page->prev = arena->tail;
		arena->tail = page;
		page->next = NULL;
	}
}

void page_free_pages(struct page_arena *arena, void *addr, size_t nr_pages)
{
	ScopedSpinlock lock(&arena->lock);

	if(!arena->page_list)
	{
		struct page_list *list = NULL;
		uintptr_t b = (uintptr_t) addr;
		for(size_t i = 0; i < nr_pages; i++, b += PAGE_SIZE)
		{
			struct page_list *l = (struct page_list *) phys_to_virt(b);
			l->page = phys_to_page(b);
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
			struct page_list *l = (struct page_list *) phys_to_virt(b);
			l->page = phys_to_page(b);
			l->next = NULL;
			list->next = l;
			l->prev = list;
			list = l;
		}
	}
}

void page_free(size_t nr_pages, void *addr)
{
	/* printk("Freeing %p\n", addr); */
	for_every_arena(&main_cpu)
	{
		if((uintptr_t) arena->start_arena <= (uintptr_t) addr && 
			(uintptr_t) arena->end_arena > (uintptr_t) addr)
		{
			page_free_pages(arena, addr, nr_pages);
			__sync_sub_and_fetch(&used_pages, nr_pages);;
		}
	}
}

size_t page_add_counter = 0;

static int page_add(struct page_arena *arena, void *__page,
	struct boot_info *info)
{
	if(page_is_used(info, __page))
		return -1;
	page_add_counter++;

	struct page_list *page = (struct page_list *) phys_to_virt(__page);
	page->next = NULL;

	append_page(arena, page);
	page->page = page_add_page(__page);

	return 0;
}

static void append_arena(struct page_cpu *cpu, struct page_arena *arena)
{
	struct page_arena **a = &cpu->arenas;

	while(*a)
		a = &(*a)->next;
	*a = arena;
}

static void page_add_region(uintptr_t base, size_t size, struct boot_info *info)
{
	while(size)
	{
		size_t area_size = min(size, 0x200000);
		struct page_arena *arena = (struct page_arena *) __ksbrk(sizeof(struct page_arena));
		assert(arena != NULL);
		memset_s(arena, 0, sizeof(struct page_arena));

		arena->free_pages = arena->nr_pages = area_size >> PAGE_SHIFT;
		arena->start_arena = (void*) base;
		arena->end_arena = (void*) (base + area_size);

		for(size_t i = 0; i < area_size; i += PAGE_SIZE)
		{
			/* If the page is being used, decrement the free_pages counter */
			if(page_add(arena, (void*) (base + i), info) < 0)
				arena->free_pages--;
		}

		append_arena(&main_cpu, arena);

		size -= area_size;
		base += area_size;
	}
}

void *efi_allocate_early_boot_mem(size_t size);

void page_init(size_t memory_size, void *(*get_phys_mem_region)(uintptr_t *base,
	uintptr_t *size, void *context), struct boot_info *info)
{
	uintptr_t region_base;
	uintptr_t region_size;
	void *context_cookie = NULL;

	printf("page: Memory size: %lu\n", memory_size);
	page_memory_size = memory_size;
	nr_global_pages = size_to_pages(memory_size);

	size_t nr_arenas = page_memory_size / 0x200000;
	if(page_memory_size % 0x200000)
		nr_arenas++;

	size_t needed_memory = nr_arenas *
		sizeof(struct page_arena) + 
		nr_global_pages * sizeof(struct page);
	void *ptr = efi_allocate_early_boot_mem(needed_memory);
	if(!ptr)
	{
		panic("Failure on page_init");
	}

	__kbrk(phys_to_virt(ptr));

	/* The context cookie is supposed to be used as a way for the
	 * get_phys_mem_region implementation to keep track of where it's at,
	 * without needing ugly global variables.
	*/

	/* Loop this call until the context cookie is NULL
	* (we must have reached the end)
	*/

	while((context_cookie = get_phys_mem_region(&region_base,
		&region_size, context_cookie)) != NULL)
	{
		/* page_add_region can't return an error value since it halts
		 * on failure
		*/
		page_add_region(region_base, region_size, info);
	}

	page_is_initialized = true;
}

extern unsigned char kernel_end;

void *kernel_break = &kernel_end;

__attribute__((malloc))
void *__ksbrk(long inc)
{
	void *ret = kernel_break;
	kernel_break = (char*) kernel_break + inc;
	return ret;
}

void __kbrk(void *break_)
{
	kernel_break = break_;
}

void free_pages(struct page *pages)
{
	struct page *next = NULL;

	for(struct page *p = pages; p != NULL; p = next)
	{
		next = p->next_un.next_allocation;
		free_page(p);
	}
}

void free_page(struct page *p)
{
	assert(p != NULL);
	assert(p->ref != 0);

	if(page_unref(p) == 0)
	{
		p->next_un.next_allocation = NULL;
		page_free(1, p->paddr);
	}
}

inline struct page *alloc_pages_nozero(size_t nr_pgs, unsigned long flags)
{
	return page_alloc(nr_pgs, flags);
}

struct page *__get_phys_pages(size_t nr_pgs, unsigned long flags)
{
	struct page *plist = NULL;
	size_t off = 0;

	for(size_t i = 0; i < nr_pgs; i++, off += PAGE_SIZE)
	{
		struct page *p = alloc_pages_nozero(1, flags);

		if(!p)
		{
			if(plist)
				free_pages(plist);

			return NULL;
		}

		p->off = off;

		if(page_should_zero(flags))
		{
			memset(phys_to_virt(p->paddr), 0, PAGE_SIZE);
		}

		if(!plist)
		{
			plist = p;
		}
		else
		{
			struct page *i = plist;
			for(; i->next_un.next_allocation != NULL; i = i->next_un.next_allocation);
			i->next_un.next_allocation = p;
		}
	}

	return plist;
}

struct page *do_alloc_pages_contiguous(size_t nr_pgs, unsigned long flags)
{
	struct page *p = alloc_pages_nozero(nr_pgs, flags);
	if(!p)
		return NULL;
	
	if(page_should_zero(flags))
	{
		memset(phys_to_virt(p->paddr), 0, nr_pgs << PAGE_SHIFT);
	}

	return p;
}

struct page *alloc_pages(size_t nr_pgs, unsigned long flags)
{
	if(unlikely(flags & PAGE_ALLOC_CONTIGUOUS))
		return do_alloc_pages_contiguous(nr_pgs, flags);
	else
		return __get_phys_pages(nr_pgs, flags);
}

namespace Page
{

void GetStats(struct page_usage *usage)
{
	usage->total_pages = nr_global_pages;
	usage->used_pages = used_pages;
}

};