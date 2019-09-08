/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _VMOBJECT_H
#define _VMOBJECT_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <carbon/vm.h>
#include <carbon/lock.h>
#include <carbon/page.h>
#include <carbon/list.h>
#include <carbon/panic.h>
#include <carbon/smart.h>

#include <libdict/rb_tree.h>
struct page;

class vm_object
{

protected:
	/* TODO: Create a list of owners when we're able to share VMOs */
	struct vm_region *owner;
	size_t nr_pages;
	struct rb_tree page_list;
	bool should_demand_page;

	void purge_pages(size_t lower_bound, size_t upper_bound, unsigned int flags, vm_object *second = nullptr);
	void update_offsets(size_t old_off);
	void destroy_tree(void (*func)(void *key, void *data));
	struct page *get_may_commit_unlocked(size_t off);
public:
	Spinlock lock;
	static constexpr bool is_refcountable = true;

	vm_object(bool should_demand_page,
		 size_t nr_pages,
		 struct vm_region *owner)
		 : owner(owner), nr_pages(nr_pages), lock()
	{
		memset((void *) &page_list, 0, sizeof(page_list));
		page_list.cmp_func = vm_cmp;
	};


	inline void set_owner(struct vm_region *_owner)
	{
		owner = _owner;
	}

	/* Base ~vm_object, deletes the tree and disposes of the pages using dispose_page() */
	virtual ~vm_object();

	/* Implemented in specializations */
	virtual void dispose_page(struct page *p) {};
	virtual int commit(size_t offset) = 0;
	virtual vm_object *create_hollow_copy() = 0;
	virtual int populate(size_t starting_off, size_t region_size);

	/* Generic functions */
	struct page *get(size_t offset);
	int resize(size_t new_size);
	vm_object *split(size_t split_point, size_t hole_size);
	void sanity_check();
	void truncate_beginning_and_resize(size_t off);
	size_t write(size_t offset, const void *src, size_t size);
	size_t read(size_t offset, void *dst, size_t size);
	size_t set_mem(size_t offset, uint8_t pattern, size_t size);

	/* add_page and remove_page are dangerous, beware! */
	int add_page(size_t page_off, struct page *page);
	struct page *remove_page(size_t page_off);
};

/* Following this is a number of specializations of the vm_object class */

class vm_object_phys : public vm_object
{
protected:
public:
	using vm_object::vm_object;
	~vm_object_phys() override;

	int commit(size_t offset) override;
	vm_object *create_hollow_copy()
	{
		vm_object_phys *p = new vm_object_phys(should_demand_page, nr_pages, owner);
		return p;
	}
};

class vm_object_mmio : public vm_object
{
private:
	struct page *pages;
public:
	using vm_object::vm_object;
	~vm_object_mmio() override
	{
		delete[] pages;
	}

	bool init(unsigned long phys);
	int commit(size_t offset) override;
	
	int populate(size_t starting_off, size_t region_size) override
	{
		/* vm_object_mmio regions don't need to be populated */
		(void) starting_off;
		(void) region_size;
		return 0;
	}

	vm_object *create_hollow_copy()
	{
		vm_object_mmio *mmio = new vm_object_mmio(should_demand_page, nr_pages, owner);
		return mmio;
	}
	
};

#endif