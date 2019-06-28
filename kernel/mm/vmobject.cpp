/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>
#include <string.h>

#include <carbon/lock.h>
#include <carbon/vmobject.h>
#include <carbon/page.h>
#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/panic.h>

int VmObject::AddPage(size_t offset, struct page *page)
{
	ScopedSpinlock l(&lock);

	page->off = offset;

	if(!page_list.Add(page))
		return -1;
	

	return 0;
}

struct page *VmObject::RemovePage(size_t page_off)
{
	ScopedSpinlock l(&lock);

	struct page *target = nullptr;
	LinkedListIterator<struct page *> starting_it;
	for(auto it = page_list.begin(); it != page_list.end(); it++)
	{
		auto page = *it;
		if(page->off == page_off)
		{
			target = page;
			starting_it = it;
			break;
		}
	}

	assert(target != nullptr);

	assert(page_list.Remove(target, starting_it) == true);

	return target;
}

int VmObject::Populate(size_t starting_off, size_t region_size)
{
	size_t nr_pages = size_to_pages(region_size);

	while(nr_pages--)
	{
		int st = Commit(starting_off);

		if(st < 0)
			return st;

		starting_off += PAGE_SIZE;
	}

	return 0;
}

struct page *VmObject::Get(size_t offset)
{
	ScopedSpinlock l(&lock);

	for(auto page : page_list)
	{
		if(page->off == offset)
			return page;
	}

	return nullptr;
}

int VmObjectPhys::Commit(size_t offset)
{
	struct page *p = alloc_pages(1, 0);

	if(!p)
		return -1;

	AddPage(offset, p);

	return 0;
}

inline bool is_included(size_t lower, size_t upper, size_t x)
{
	if(x >= lower && x < upper)
		return true;
	return false;
}

inline bool is_excluded(size_t lower, size_t upper, size_t x)
{
	if(x < lower || x > upper)
		return true;
	return false;
}


#define PURGE_SHOULD_FREE	(1 << 0)
#define	PURGE_EXCLUDE		(1 << 1)


void VmObject::PurgePages(size_t lower_bound, size_t upper_bound, unsigned int flags, VmObject *second)
{
	ScopedSpinlock l(&lock);

	auto node = page_list.GetHead();

	bool should_free = flags & PURGE_SHOULD_FREE;
	bool exclusive = flags & PURGE_EXCLUDE;

	assert(!(should_free && second != nullptr));

	bool (*compare_function)(size_t, size_t, size_t) = is_included;

	if(exclusive)
		compare_function = is_excluded;

	while(node != nullptr)
	{
		auto p = node->data;

		if(compare_function(lower_bound, upper_bound, p->off))
		{
			LinkedListIterator<struct page *> it(node);
			
			node = node->next;

			assert(page_list.Remove(p, it) != false);

			p->next_un.next_virtual_region = nullptr;

			/* TODO: Add a virtual function to do this */
			if(should_free)
			{
				free_page(p);
			}

			if(second)
				second->AddPage(p->off, p);
		}
		else
		{
			node = node->next;
		}
	}
}

int VmObject::Resize(size_t new_size)
{
	nr_pages = new_size >> PAGE_SHIFT;
	/* Subtract PAGE_SIZE from new_size because is_excluded does x > upper
	 * (TL;DR so we don't leak a page) */
	PurgePages(0, new_size - PAGE_SIZE, PURGE_SHOULD_FREE | PURGE_EXCLUDE);

	return 0;
}

void VmObject::UpdateOffsets(size_t off)
{
	ScopedSpinlock l(&lock);

	for(auto page : page_list)
	{
		page->off -= off;
	}
}

VmObject *VmObject::Split(size_t split_point, size_t hole_size)
{
	size_t split_point_pgs = split_point >> PAGE_SHIFT;
	size_t hole_size_pgs = hole_size >> PAGE_SHIFT;

	VmObject *second_vmo = CreateHollowCopy();

	if(!second_vmo)
		return nullptr;

	second_vmo->nr_pages -= split_point_pgs + hole_size_pgs;;

	auto max = hole_size + split_point;

	PurgePages(split_point, max, PURGE_SHOULD_FREE);
	PurgePages(max, nr_pages << PAGE_SHIFT, 0, second_vmo);
	second_vmo->UpdateOffsets(split_point + hole_size);

	nr_pages -= hole_size_pgs + second_vmo->nr_pages;

	return second_vmo;
}

void VmObject::SanityCheck()
{
	ScopedSpinlock l(&lock);

	for(auto p : page_list)
	{
		if(p->off > (nr_pages) << PAGE_SHIFT)
		{
			printf("Bad vmobject: p->off > nr_pages << PAGE_SHIFT.\n");
			printf("struct page: %p\n", p);
			printf("Offset: %lx\n", p->off);
			printf("Size: %lx\n", nr_pages << PAGE_SHIFT);
			panic("bad vmobject");
		}

		if(p->ref == 0)
		{
			printf("Bad vmobject:: p->ref == 0.\n");
			printf("struct page: %p\n", p);
			panic("bad vmobject");
		}

		printf("Page: %p\n", p->paddr);
	}	
}

void VmObject::TruncateBeginningAndResize(size_t off)
{
	PurgePages(0, off, PURGE_SHOULD_FREE);
	UpdateOffsets(off);
	nr_pages -= (off >> PAGE_SHIFT);
}

bool VmObjectMmio::Init(unsigned long phys)
{
	auto original_phys = phys;
	auto pages = new struct page[nr_pages];
	if(!pages)
		return false;
	
	memset(pages, 0, sizeof(struct page) * nr_pages);

	for(size_t i = 0; i < nr_pages; i++)
	{
		pages[i].paddr = (void *) phys;
		pages[i].off = phys - original_phys;
		pages[i].ref = 1;
		pages[i].flags = PAGE_FLAG_DONT_FREE;
		if(i != 0)	pages[i - 1].next_un.next_allocation = &pages[i];

		phys += PAGE_SIZE;
	}

	assert(PageToList(&page_list, pages) != false);

	return true;
}

int VmObjectMmio::Commit(size_t offset)
{
	return 0;
}