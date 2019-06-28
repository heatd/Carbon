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
#include <carbon/list.h>

struct page;

class VmObject
{

protected:
	/* TODO: Create a list of owners when we're able to share VMOs */
	struct vm_region *owner;
	size_t nr_pages;
	LinkedList<struct page *> page_list;
	bool should_demand_page;

	int AddPage(size_t page_off, struct page *page);
	struct page *RemovePage(size_t page_off);
	void PurgePages(size_t lower_bound, size_t upper_bound, unsigned int flags, VmObject *second = nullptr);
	void UpdateOffsets(size_t old_off);
public:
	Spinlock lock;
	static constexpr bool is_refcountable = true;
	
	static inline bool PageToList(LinkedList<struct page *> *list, struct page *pages)
	{
		for(struct page *p = pages; p != nullptr; p = p->next_un.next_allocation)
		{
			if(!list->Add(p))
			{
				/* Undo stuff if something fails */
				for(struct page *page = pages; page != p; page = page->next_un.next_allocation)
					list->Remove(page);
				
				return false;
			}
		}

		return true;
	}
	
	int Populate(size_t starting_off, size_t region_size);

	VmObject(bool should_demand_page,
		 size_t nr_pages,
		 struct vm_region *owner,
		 struct page *init_pages)
		 : owner(owner), nr_pages(nr_pages), page_list(), lock()
	{
		assert(PageToList(&page_list, init_pages) != false);
	};

	VmObject(bool should_demand_page,
		 size_t nr_pages,
		 struct vm_region *owner,
		 LinkedList<struct page *> init_pages)
		 : owner(owner), nr_pages(nr_pages), page_list(), lock()
	{
		assert(page_list.Copy(&init_pages) != false);
	};


	inline void SetOwner(struct vm_region *_owner)
	{
		owner = _owner;
	}

	virtual ~VmObject(){};
	int Fork();
	virtual int Commit(size_t offset) = 0;
	struct page *Get(size_t offset);
	int Resize(size_t new_size);
	VmObject *Split(size_t split_point, size_t hole_size);
	virtual VmObject *CreateHollowCopy() = 0;
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

		for(auto it = page_list.begin(); it != page_list.end();)
		{
			auto t = it;
			it++;
	
			struct page *page = *t;
			page_list.Remove(page);
			free_page(page);

		}

		lock.Unlock();
	}

	int Commit(size_t offset);
	VmObject *CreateHollowCopy()
	{
		VmObjectPhys *p = new VmObjectPhys(should_demand_page, nr_pages, owner, (struct page *) nullptr);
		return p;
	}
};

class VmObjectMmio : public VmObject
{

public:
	using VmObject::VmObject;
	~VmObjectMmio(){}
	bool Init(unsigned long phys);
	int Commit(size_t offset);
	VmObject *CreateHollowCopy()
	{
		VmObjectMmio *mmio = new VmObjectMmio(should_demand_page, nr_pages, owner, (struct page *) nullptr);
		return mmio;
	}
	
};

#endif