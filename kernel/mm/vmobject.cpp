/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>

#include <carbon/lock.h>
#include <carbon/vmobject.h>
#include <carbon/page.h>
#include <carbon/vm.h>
#include <carbon/memory.h>

int VmObject::AddPage(size_t offset, struct page *page)
{
	spin_lock(&lock);

	page->off = offset;

	struct page **pp = &page_list;
	while(*pp)
		pp = &(*pp)->next_un.next_virtual_region;

	*pp = page;

	spin_unlock(&lock);

	return 0;
}

struct page *VmObject::RemovePage(size_t page_off)
{
	spin_lock(&lock);

	struct page *ret = nullptr;

	if(page_list != nullptr)
	{
		if(page_list->off == page_off)
		{
			ret = page_list;
			page_list = page_list->next_un.next_virtual_region;
		}
	}
	else
	{
		for(struct page *p = page_list; p->next_un.next_virtual_region;
		    p = p->next_un.next_virtual_region)
		{
			if(p->next_un.next_virtual_region->off == page_off)
			{
				ret = p->next_un.next_virtual_region;
				p->next_un.next_virtual_region = ret->next_un.next_virtual_region;
			}
		}
	}

	spin_unlock(&lock);

	return ret;
}

int VmObject::Populate(size_t starting_off, size_t region_size)
{
	size_t nr_pages = size_to_pages(region_size);

	printf("populating %u pages\n", nr_pages);
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
	spin_lock(&lock);

	struct page *p = page_list;

	while(p)
	{
		if(p->off == offset)
		{
			spin_unlock(&lock);
			return p;
		}

		p = p->next_un.next_virtual_region;
	}

	spin_unlock(&lock);

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