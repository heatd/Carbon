/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _VMOBJECT_H
#define _VMOBJECT_H

#include <stddef.h>

#include <carbon/vm.h>
#include <carbon/lock.h>
#include <carbon/page.h>

struct page;

class VmObject
{

protected:
	/* TODO: Create a list of owners when we're able to share VMOs */
	struct vm_region *owner;
	size_t nr_pages;
	struct page *page_list;
	bool should_demand_page;

	int AddPage(size_t page_off, struct page *page);
	struct page *RemovePage(size_t page_off);
	void PurgePages(size_t lower_bound, size_t upper_bound, unsigned int flags, VmObject *second = nullptr);
	void UpdateOffsets(size_t old_off);
public:
	Spinlock lock;
	static constexpr bool is_refcountable = true;
	int Populate(size_t starting_off, size_t region_size);

	VmObject(bool should_demand_page,
		 size_t nr_pages,
		 struct vm_region *owner,
		 struct page *init_pages)
		 : owner(owner), nr_pages(nr_pages), page_list(init_pages), lock()
	{};

	virtual ~VmObject(){};
	int Fork();
	virtual int Commit(size_t offset) = 0;
	struct page *Get(size_t offset);
	int Resize(size_t new_size);
	VmObject *Split(size_t split_point, size_t hole_size);
	virtual VmObject *CreateCopy() = 0;
	void SanityCheck();
	void TruncateBeginningAndResize(size_t off);
};

/* Following this is a number of specializations of the VmObject class */

class VmObjectPhys : public VmObject
{

public:
	using VmObject::VmObject;
	~VmObjectPhys()
	{
		lock.Lock();

		free_pages(page_list);

		lock.Unlock();
	}

	int Commit(size_t offset);
	VmObject *CreateCopy()
	{
		VmObjectPhys *p = new VmObjectPhys(should_demand_page, nr_pages, owner, page_list);
		return p;
	}
};

#endif