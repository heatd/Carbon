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
#include <carbon/syscall_utils.h>

int vm_object::add_page(size_t offset, struct page *page)
{
	scoped_spinlock l(&lock);

	page->off = offset;

	auto res = rb_tree_insert(&page_list, (void *) offset);
	if(!res.inserted)
	{
		assert(res.datum_ptr != nullptr);
		return -1;
	}

	*res.datum_ptr = (void *) page;

	return 0;
}

struct page *vm_object::remove_page(size_t page_off)
{
	scoped_spinlock l(&lock);

	struct page *target = nullptr;
	auto res = rb_tree_remove(&page_list, (const void *) page_off);

	assert(res.removed == true);

	return (target = (struct page *) res.datum);
}

int vm_object::populate(size_t starting_off, size_t region_size)
{
	size_t nr_pages = size_to_pages(region_size);

	while(nr_pages--)
	{
		int st = commit(starting_off);

		if(st < 0)
			return st;

		starting_off += PAGE_SIZE;
	}

	return 0;
}

struct page *vm_object::get(size_t offset)
{
	lock.Lock();

	assert((offset & (PAGE_SIZE - 1)) == 0);

	void **pp = rb_tree_search(&page_list, (const void *) offset);

	return pp ? (struct page *) *pp : (lock.Unlock(), nullptr);
}

int vm_object_phys::commit(size_t offset)
{
	struct page *p = alloc_pages(1, 0);

	if(!p)
		return -1;
	if(add_page(offset, p) < 0)
	{
		free_page(p);
		return -1;
	}

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


void vm_object::purge_pages(size_t lower_bound, size_t upper_bound,
			    unsigned int flags, vm_object *second)
{
	scoped_spinlock l(&lock);

	rb_itor it{&page_list, nullptr};

	bool should_free = flags & PURGE_SHOULD_FREE;
	bool exclusive = flags & PURGE_EXCLUDE;

	assert(!(should_free && second != nullptr));

	bool (*compare_function)(size_t, size_t, size_t) = is_included;

	if(exclusive)
		compare_function = is_excluded;

	bool node_valid = rb_itor_first(&it);

	while(node_valid)
	{
		struct page *p = (struct page *) *rb_itor_datum(&it);

		if(compare_function(lower_bound, upper_bound, p->off))
		{
			rb_itor_remove(&it);

			node_valid = rb_itor_search_ge(&it, (const void *) p->off);

			p->next_un.next_virtual_region = nullptr;

			/* TODO: Add a virtual function to do this */
			if(should_free)
			{
				free_page(p);
			}

			if(second)
				second->add_page(p->off, p);
		}
		else
		{
			node_valid = rb_itor_next(&it);
		}
	}
}

int vm_object::resize(size_t new_size)
{
	nr_pages = new_size >> PAGE_SHIFT;
	/* Subtract PAGE_SIZE from new_size because is_excluded does x > upper
	 * (TL;DR so we don't leak a page) */
	purge_pages(0, new_size - PAGE_SIZE, PURGE_SHOULD_FREE | PURGE_EXCLUDE);

	return 0;
}

void vm_object::update_offsets(size_t off)
{
	scoped_spinlock l(&lock);

	rb_itor it{&page_list, nullptr};

	bool node_valid = rb_itor_first(&it);

	while(node_valid)
	{
		auto page = (struct page *) *rb_itor_datum(&it);
		page->off -= off;

		node_valid = rb_itor_next(&it);
	}
}

vm_object *vm_object::split(size_t split_point, size_t hole_size)
{
	size_t split_point_pgs = split_point >> PAGE_SHIFT;
	size_t hole_size_pgs = hole_size >> PAGE_SHIFT;

	vm_object *second_vmo = create_hollow_copy();

	if(!second_vmo)
		return nullptr;

	second_vmo->nr_pages -= split_point_pgs + hole_size_pgs;;

	auto max = hole_size + split_point;

	purge_pages(split_point, max, PURGE_SHOULD_FREE);
	purge_pages(max, nr_pages << PAGE_SHIFT, 0, second_vmo);
	second_vmo->update_offsets(split_point + hole_size);

	nr_pages -= hole_size_pgs + second_vmo->nr_pages;

	return second_vmo;
}

void vm_object::sanity_check()
{
	scoped_spinlock l(&lock);

	rb_itor it{&page_list, nullptr};

	bool node_valid = rb_itor_first(&it);

	while(node_valid)
	{
		auto p = (struct page *) *rb_itor_datum(&it);
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

		node_valid = rb_itor_next(&it);
	}
}

void vm_object::truncate_beginning_and_resize(size_t off)
{
	purge_pages(0, off, PURGE_SHOULD_FREE);
	update_offsets(off);
	nr_pages -= (off >> PAGE_SHIFT);
}

bool vm_object_mmio::init(unsigned long phys)
{
	auto original_phys = phys;
	pages = new struct page[nr_pages];
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
		add_page(i << PAGE_SHIFT, &pages[i]);
	}

	return true;
}

int vm_object_mmio::commit(size_t offset)
{
	return -1;
}

void vm_object::destroy_tree(void (*func)(void *, void *))
{
	rb_tree_free(&page_list, func);
}

vm_object::~vm_object()
{
}

vm_object_phys::~vm_object_phys()
{
	destroy_tree([](void *key, void *data)
	{
		free_page((struct page *) data);
	});
}

struct page *vm_object::get_may_commit_unlocked(size_t off)
{
	auto p = get(off);
	if(!p)
	{
		if(commit(off) < 0)
			return nullptr;
		return get(off);
	}

	return p;
}

size_t vm_object::write(size_t offset, const void *src, size_t size)
{
	size_t written = 0;

	const uint8_t *s = (const uint8_t *) src;

	while(size != 0)
	{
		size_t misalignment = offset & (PAGE_SIZE - 1);

		auto page = get_may_commit_unlocked(offset - misalignment);
		if(!page)
		{
			lock.Unlock();
			return written ? written : -1;
		}

		size_t to_write = PAGE_SIZE - misalignment < size ? PAGE_SIZE - misalignment : size;
		unsigned long paddr = (unsigned long) page->paddr + misalignment;
		memcpy(phys_to_virt(paddr), s, to_write);

		written += to_write;
		s += to_write;
		offset += to_write;
		size -= to_write;
		lock.Unlock();
	}

	return written;
}

size_t vm_object::set_mem(size_t offset, uint8_t pattern, size_t size)
{
	size_t written = 0;

	while(size != 0)
	{
		size_t misalignment = offset & (PAGE_SIZE - 1);

		auto page = get_may_commit_unlocked(offset - misalignment);
		if(!page)
		{
			lock.Unlock();
			return written ? written : -1;
		}

		size_t to_write = PAGE_SIZE - misalignment < size ? PAGE_SIZE - misalignment : size;
		unsigned long paddr = (unsigned long) page->paddr + misalignment;
		memset(phys_to_virt(paddr), pattern, to_write);

		written += to_write;
		offset += to_write;
		size -= to_write;
		lock.Unlock();
	}

	return written;
}

size_t vm_object::read(size_t offset, void *dst, size_t size)
{
	size_t been_read = 0;

	uint8_t *d = (uint8_t *) dst;

	while(size != 0)
	{
		size_t misalignment = offset & (PAGE_SIZE - 1);

		auto page = get_may_commit_unlocked(offset - misalignment);
		if(!page)
		{
			lock.Unlock();
			return been_read ? been_read : -1;
		}

		size_t to_read = PAGE_SIZE - misalignment < size ? PAGE_SIZE - misalignment : size;
		unsigned long paddr = (unsigned long) page->paddr + misalignment;
		memcpy(d, phys_to_virt(paddr), to_read);

		been_read += to_read;
		d += to_read;
		offset += to_read;
		size -= to_read;
		lock.Unlock();
	}

	return been_read;
}

cbn_status_t sys_cbn_vmo_create(size_t size, cbn_handle_t *out)
{
	auto& handle_table = get_current_process()->get_handle_table();

	auto vmo = new vm_object_phys{true, size_to_pages(size), nullptr};
	if(!vmo)
		return CBN_STATUS_OUT_OF_MEMORY;
	
	handle *h = new handle{vmo, handle::vmo_object_type, get_current_process()};
	if(!h)
	{
		delete vmo;
		return CBN_STATUS_OUT_OF_MEMORY;
	}

	auto handle_id = handle_table.allocate_handle(h);
	if(handle_id == CBN_INVALID_HANDLE)
	{
		delete h;
		return CBN_STATUS_OUT_OF_MEMORY;
	}

	if(copy_to_user(out, &handle_id, sizeof(handle_id)) < 0)
		return CBN_STATUS_SEGFAULT;

	return CBN_STATUS_OK;
}
