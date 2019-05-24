/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/panic.h>
#include <carbon/vmobject.h>

#include <libdict/dict.h>

#define KADDR_SPACE_SIZE	0x800000000000
#define KADDR_START		0xffff800000000000

struct address_space kernel_address_space = {};

int vm_cmp(const void* k1, const void* k2)
{
	if(k1 == k2)
		return 0;

        return (unsigned long) k1 < (unsigned long) k2 ? -1 : 1; 
}

struct vm_region *vm_reserve_region(struct address_space *as,
				    unsigned long start, size_t size)
{
	struct vm_region *region = (struct vm_region *) malloc(sizeof(*region));
	if(!region)
		return NULL;

	region->start = start;
	region->size = size;
	region->perms = 0;

	dict_insert_result res = rb_tree_insert(as->area_tree,
						(void *) start);

	if(res.inserted == false)
	{
		free(region);
		return NULL;
	}

	*res.datum_ptr = region;

	return region; 
}

struct vm_region *vm_allocate_region(struct address_space *as,
				     unsigned long min, size_t size)
{
	if(min < as->start)
		min = as->start;

	rb_itor *it = rb_itor_new(as->area_tree);
	bool node_valid;
	unsigned long last_end = min;
	struct vm_region *f = nullptr;

	if(min != as->start)
		node_valid = rb_itor_search_ge(it, (const void *) min);
	else
	{
		node_valid = rb_itor_first(it);
	}

	if(!node_valid)
			goto done;
	

	/* Check if there's a gap between the first node
	 * and the start of the address space
	*/

	f = (struct vm_region *) *rb_itor_datum(it);

#if DEBUG_VM
	printf("Tiniest node: %016lx\n", f->start);
#endif
	if(f->start - min >= size)
	{
#if DEBUG_VM
		printf("gap [%016lx - %016lx]\n", min, f->start);
#endif
		goto done;
	}
	
	while(node_valid)
	{
		struct vm_region *f = (struct vm_region *) *rb_itor_datum(it);
		last_end = f->start + f->size;
 
		node_valid = rb_itor_next(it);
		if(!node_valid)
			break;

		struct vm_region *vm = (struct vm_region *) *rb_itor_datum(it);

		if(vm->start - last_end >= size && min <= vm->start)
			break;
	}

done:
	rb_itor_free(it);

	last_end = last_end < min ? min : last_end;

#if DEBUG_VM
	printf("Ptr: %lx\nSize: %lx\n", last_end, size);
#endif
	return vm_reserve_region(as, last_end, size);
}

void vm_init(void)
{
	kernel_address_space.area_tree = rb_tree_new(vm_cmp);
	kernel_address_space.start = KADDR_START;
	kernel_address_space.end = UINTPTR_MAX;

	assert(kernel_address_space.area_tree != NULL);

	struct kernel_limits limits;

	get_kernel_limits(&limits);

	limits.end_virt = (unsigned long) ksbrk(0);

	size_t kernel_size = limits.end_virt - limits.start_virt;

	if(kernel_size & (PAGE_SIZE - 1))
		kernel_size += PAGE_SIZE - (kernel_size & (PAGE_SIZE - 1));

	if(vm_reserve_region(&kernel_address_space, limits.start_virt, kernel_size) == NULL)
	{
		panic("boot vm_reserve_region failed");
	}

	malloc_reserve_memory_space();
}

void vm_remove_region(struct address_space *as, struct vm_region *region)
{
	dict_remove_result res = rb_tree_remove(as->area_tree,
						 (const void *) region->start);
	assert(res.removed == true);
}

void vm_destroy_region(struct address_space *as, struct vm_region *region)
{
	vm_remove_region(as, region);	
	/* Slowly destroy the vm region object now */

	if(region->vmo)
		delete region->vmo;

	free(region);
}

void vm_assign_vmo(struct vm_region *region, VmObject *vmo)
{
	region->vmo = vmo;
}

int vm_update_mapping(struct vm_region *region, size_t off, size_t len)
{
	size_t nr_pages = size_to_pages(len);
	unsigned long start = region->start + off;

	while(nr_pages--)
	{
		struct page *p = region->vmo->Get(off);

		assert(p != nullptr);

		if(!map_phys_to_virt(start + off, (unsigned long) p->paddr, region->perms))
			return -1;

		off += PAGE_SIZE;
		region->vmo->lock.Unlock();
	}

	return 0;
}

namespace Vm
{

struct vm_region *FindRegion(void *addr, rb_tree *tree)
{
	rb_itor *it = rb_itor_new(tree);

	if(!rb_itor_search_le(it, addr))
		return NULL;
	
	while(true)
	{
		struct vm_region *region = (struct vm_region *) *rb_itor_datum(it);
		if(region->start <= (unsigned long) addr
			&& region->start + region->size > (unsigned long) addr)
		{
			rb_itor_free(it);
			return region;
		}

		if(!rb_itor_next(it))
		{
			break;
		}
	}

	rb_itor_free(it);
	return NULL;
}

void VmFault::Dump()
{
	printf("Page fault at %lx, accessing %lx ", ip, fault_address);
	printf("Access info: ");

	if(is_write)
		printf("W ");
	if(!is_write)
		printf("R ");
	if(is_exec)
		printf("X ");
	if(is_user)
		printf("U ");
	printf(" \n");
}

unsigned long VmFault::FlagsToPerms()
{
	unsigned long perms = 0;

	if(is_write)
		perms |= VM_PROT_WRITE;
	if(is_exec)
		perms |= VM_PROT_EXEC;
	if(is_user)
		perms |= VM_PROT_USER;
	return perms;
}

enum VmFaultStatus VmFault::TryToMapPage(struct vm_region *region)
{
	auto vmo = region->vmo;
	unsigned long fault_addr_aligned = fault_address & ~(PAGE_SIZE - 1);
	size_t off = fault_addr_aligned - region->start;
	
	auto page = vmo->Get(off);

	if(!page)
	{
		vmo->lock.Unlock();
		if(vmo->Commit(off) < 0)
			return VmFaultStatus::VM_SEGFAULT;
		page = vmo->Get(off);
	}

	if(!map_phys_to_virt(region->start + off, (unsigned long ) page->paddr, region->perms))
	{
		return VmFaultStatus::VM_SEGFAULT;
	}

	vmo->lock.Unlock();
	return VmFaultStatus::VM_OK;
}

enum VmFaultStatus VmFault::Handle()
{
	/* TODO: Remove this. */
	assert(!kernel_address_space.lock.IsLocked());

	ScopedSpinlock l(&kernel_address_space.lock);
	auto region = FindRegion((void *) fault_address, kernel_address_space.area_tree);

	if(!region)
		return VmFaultStatus::VM_SEGFAULT;
	
	unsigned long perms = FlagsToPerms();

	if((region->perms & perms) != perms)
	{
		/* Oh oh, the perms have been violated, segfault */
		return VmFaultStatus::VM_SEGFAULT;
	}

	return TryToMapPage(region);
}

struct vm_region *AllocateRegionInternal(struct address_space *as, unsigned long min, size_t size)
{
	return vm_allocate_region(as, min, size);
}

struct vm_region *MmapInternal(struct address_space *as, unsigned long min, size_t size, unsigned long flags)
{
	struct vm_region *reg = AllocateRegionInternal(as, min, size);

	if(!reg)
		return nullptr;

	reg->perms = flags;

	VmObjectPhys *phys = new VmObjectPhys(false, size_to_pages(size), reg, nullptr);

	if(!phys)
	{
		vm_destroy_region(as, reg);
		return nullptr;
	}

	vm_assign_vmo(reg, phys);
	
	if(flags & VM_PROT_USER)
		return reg;

	if(phys->Populate(0, size) < 0)
	{
		delete phys;
		vm_destroy_region(as, reg);
		return nullptr;
	}

	if(vm_update_mapping(reg, 0, size) < 0)
	{
		delete phys;
		vm_destroy_region(as, reg);
		return nullptr;
	}

	return reg;	
}

void *mmap(struct address_space *as, unsigned long min, size_t size, unsigned long flags)
{
	struct vm_region *region = MmapInternal(as, min, size, flags);
	if(!region)	return nullptr;
	return (void *) region->start;
}

int vm_add_region(struct address_space *as, struct vm_region *region)
{
	auto res = rb_tree_insert(as->area_tree, (void *) region->start);

	if(!res.inserted)
		return -1;
	*res.datum_ptr = (void *) region;

	return 0;
}

int munmap(struct address_space *as, void *__addr, size_t size)
{
	unsigned long addr = (unsigned long) __addr;
	auto limit = addr + size;

	ScopedSpinlock l(&as->lock);

	while(addr < limit)
	{
		auto region = FindRegion((void *) addr, as->area_tree);
		if(!region)
		{
			return -EINVAL;
		}
		
		size_t to_shave_off = 0;
		if(region->start == addr)
		{
			to_shave_off = size < region->size ? size : region->size;

			if(to_shave_off != region->size)
			{
				vm_remove_region(as, region);

				region->start += to_shave_off;
				region->size -= to_shave_off;

				if(vm_add_region(as, region) < 0)
					return -ENOMEM;
				region->vmo->TruncateBeginningAndResize(to_shave_off);
				region->vmo->SanityCheck();
			}
			else
			{
				vm_destroy_region(as, region);
			}
		}
		else if(region->start < addr)
		{
			auto offset = addr - region->start;
			auto remainder = region->size - offset;
			to_shave_off = size < remainder ? size : remainder;

			if(to_shave_off != remainder)
			{
				auto second_region_start = addr + to_shave_off;
				auto second_region_size = remainder - to_shave_off;

				auto new_region = vm_reserve_region(as,
						second_region_start,
						second_region_size);

				if(!new_region)
				{
					return -ENOMEM;
				}

				vm_remove_region(as, region);

				VmObject *second = region->vmo->Split(offset, to_shave_off);
				if(!second)
				{
					vm_remove_region(as, new_region);
					return -ENOMEM;
				}

				new_region->vmo = second;

				/* The original region's size is offset */
				region->size = offset;

			}
			else
			{
				region->vmo->Resize(region->size - to_shave_off);
				region->size -= to_shave_off;
			}
		}

		unmap_page_range(NULL, (void *) addr, to_shave_off);

		addr += to_shave_off;
		size -= to_shave_off;
	}

	return 0;
}


/* TODO: Implement mprotect */
}