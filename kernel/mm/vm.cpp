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
	assert((start & (PAGE_SIZE-1)) == 0);
	size = size_to_pages(size) << PAGE_SHIFT;
	struct vm_region *region = (struct vm_region *) zalloc(sizeof(*region));

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
#define DEBUG_VM 0
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

void vm_assign_vmo(struct vm_region *region, vm_object *vmo)
{
	region->vmo = vmo;
}

int vm_update_mapping(struct address_space *as, struct vm_region *region, size_t off, size_t len)
{
	size_t nr_pages = size_to_pages(len);
	unsigned long start = region->start + off;

	while(nr_pages--)
	{
		struct page *p = region->vmo->get(off);

		assert(p != nullptr);

		if(!__map_phys_to_virt(as->arch_priv, start + off,
				       (unsigned long) p->paddr, region->perms))
		{
			region->vmo->lock.Unlock();
			return -1;
		}

		off += PAGE_SIZE;
		region->vmo->lock.Unlock();
	}

	return 0;
}

void kasan_alloc_shadow(unsigned long addr, size_t size, bool accessible);

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
	size_t vmo_off = fault_addr_aligned - region->start + region->off;
	size_t off = fault_addr_aligned - region->start;
	
	auto page = vmo->get(vmo_off);

	if(!page)
	{
		if(vmo->commit(vmo_off) < 0)
		{
			printf("commit failed\n");
			return VmFaultStatus::VM_SEGFAULT;
		}
	
		page = vmo->get(vmo_off);

		if(!page)
			return VmFaultStatus::VM_SEGFAULT;
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
	auto address_space = Vm::get_current_address_space();

	scoped_spinlock l{&address_space->lock};
	auto region = FindRegion((void *) fault_address, address_space->area_tree);

	if(!region)
		return VmFaultStatus::VM_SEGFAULT;
	
	unsigned long perms = FlagsToPerms();

	if((region->perms & perms) != perms)
	{
		printf("bad perms\n");
		/* Oh oh, the perms have been violated, segfault */
		return VmFaultStatus::VM_SEGFAULT;
	}

	return TryToMapPage(region);
}

struct vm_region *AllocateRegionInternal(struct address_space *as, unsigned long min, size_t size)
{
	return vm_allocate_region(as, min, size);
}

#define VM_PERM_FLAGS_MASK		(VM_PROT_USER | VM_PROT_WRITE | VM_PROT_EXEC)

struct vm_region *MmapInternal(struct address_space *as, unsigned long min, size_t size,
			       unsigned long flags, vm_object *vmo)
{
	scoped_spinlock guard{&as->lock};
	size = size_to_pages(size) << PAGE_SHIFT;
	struct vm_region *reg = AllocateRegionInternal(as, min, size);

	if(!reg)
		return nullptr;

	reg->perms = flags & VM_PERM_FLAGS_MASK;

	vmo->set_owner(reg);

	vm_assign_vmo(reg, vmo);
	
	if(flags & VM_PROT_USER)
		return reg;

	if(vmo->populate(0, size) < 0)
	{
		vm_destroy_region(as, reg);
		return nullptr;
	}

	if(vm_update_mapping(as, reg, 0, size) < 0)
	{
		vm_destroy_region(as, reg);
		return nullptr;
	}

#if 0
	if(as == &kernel_address_space)
		kasan_alloc_shadow(reg->start, reg->size, true);
#endif

	return reg;	
}

void *mmap(struct address_space *as, unsigned long min, size_t size, unsigned long flags)
{
	vm_object_phys *phys = new vm_object_phys(false, size_to_pages(size), nullptr);

	if(!phys)
	{
		return nullptr;
	}

	struct vm_region *region = MmapInternal(as, min, size, flags, phys);
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

	scoped_spinlock l(&as->lock);

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
				region->vmo->truncate_beginning_and_resize(to_shave_off);
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

				vm_object *second = region->vmo->split(offset, to_shave_off);
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
				region->vmo->resize(region->size - to_shave_off);
				region->size -= to_shave_off;
			}
		}

		unmap_page_range(NULL, (void *) addr, to_shave_off);

		addr += to_shave_off;
		size -= to_shave_off;
	}

	return 0;
}

void *MmioMap(struct address_space *as, unsigned long phys, unsigned long min,
	      size_t size, unsigned long flags)
{
	vm_object_mmio *vmo = new vm_object_mmio(false, size_to_pages(size), nullptr);

	if(!vmo)
		return nullptr;

	if(!vmo->init(phys))
	{
		delete vmo;
		return nullptr;
	}

	struct vm_region *region = Vm::MmapInternal(as, 0, size, flags, vmo);

	if(!region)
	{
		return nullptr;
	}
	else
		return (void *) region->start;
}

bool ForEveryRegionVisit(const void *key, void *region, void *caller_data)
{
	auto func = (bool(*) (struct vm_region *)) caller_data;
	return func((struct vm_region *) region);
}

void ForEveryRegion(struct address_space *as, bool (*func)(struct vm_region *region))
{
	rb_tree_traverse(as->area_tree, ForEveryRegionVisit, (void *) func);
}

void *allocate_stack(struct address_space *as, size_t size, unsigned long flags)
{
	/* Infer if it's a user stack from the prots */
	unsigned long min = (flags & VM_PROT_USER) ? (unsigned long) vm::limits::user_stack_min : 0;
	auto mem = Vm::mmap(as, min, size, flags);
	if(!mem)
		return nullptr;
	return (void *)((char *) mem + size);
}
/* TODO: Implement mprotect */

void *map_file(struct address_space *as, void *addr_hint,
		      unsigned long flags, unsigned long prot,
		      size_t size, size_t off, inode *ino)
{
	bool is_fixed = flags == MAP_FILE_FIXED;
	struct vm_region *region = nullptr;

	if(is_fixed)
	{
		region = vm_reserve_region(as, (unsigned long) addr_hint, size);
		if(!region)
			return nullptr;
	}
	else
		panic("implement");
	
	region->perms = prot;
	region->off = off;
	region->vmo = ino->i_pages;

	return (void *) region->start;
}

}