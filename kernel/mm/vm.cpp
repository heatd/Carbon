/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/panic.h>
#include <carbon/vmobject.h>
#include <carbon/syscall_utils.h>
#include <carbon/fs/file.h>

#include <carbon/public/vm.h>

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
		region->vmo->unref();

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
	vmo_off &= ~(PAGE_SIZE - 1);
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

bool is_region_span_free_unlocked(struct address_space *as, void *__start, size_t length)
{
	rb_itor it{as->area_tree, nullptr};
	unsigned long start = (unsigned long) __start;
	unsigned long end = start + length;

	if(!rb_itor_search_ge(&it, __start))
		return true;

	struct vm_region *region = (struct vm_region *) *rb_itor_datum(&it);
	if(region->start >= end)
		return true;
	return false;
}

bool is_valid_user_range(struct address_space *as, unsigned long addr, unsigned long length)
{
	if((unsigned long) vm::limits::user_min > addr)
		return false;
	if((unsigned long) vm::limits::user_max < addr)
		return false;
	if(as->start > addr)
		return false;
	if(as->end < addr)
		return false;
	if(as->end < (addr + length))
		return false;

	return true;
}

}

cbn_status_t cbn_mmap_get_vm_object_for_map(cbn_handle_t vmo_handle, long flags,
					   vm_object*& target_vmo)
{
	if(flags & MAP_FLAG_FILE)
	{
		/* Note: handle points to file class, file class(which is kinda the same as a
		 * file description) points to the inode, inode has a vm object that
		 * is allocated on demand.
		*/
		auto f = get_handle_from_handle_id(vmo_handle, handle::file_object_type);
		if(!f)
			return CBN_STATUS_INVALID_HANDLE;
	
		file *_file = static_cast<file *>(f->get_object());
		auto ino = _file->get_inode();

		if(!S_ISREG(ino->i_mode))
			return CBN_STATUS_INVALID_HANDLE;

		/* TODO: Add a lock for the ino since this needs to be atomic */
		if(!ino->create_vmobject_if_needed())
			return CBN_STATUS_OUT_OF_MEMORY;
		
		ino->ref();
		ino->i_pages->ref();
		target_vmo = ino->i_pages;
	}
	else
	{
		auto h = get_handle_from_handle_id(vmo_handle, handle::vmo_object_type);
		if(!h)
			return CBN_STATUS_INVALID_HANDLE;
		
		target_vmo = static_cast<vm_object *>(h->get_object());
		target_vmo->ref();
	}

	return CBN_STATUS_OK;
}

constexpr unsigned long cbn_mmap_to_vm_prots(long prot)
{
	/* TODO: Implement MAP_PROT_READ */
	return ((prot & MAP_PROT_EXEC) ? VM_PROT_EXEC : 0) |
	       ((prot & MAP_PROT_WRITE) ? VM_PROT_WRITE : 0) |
		VM_PROT_USER;
}

cbn_status_t cbn_mmap_in_place(address_space *address_space, void *hint, size_t length, size_t off,
			      long prot, vm_region *&out_region)
{
	/* Check for page alignment */
	if((unsigned long) hint & (PAGE_SIZE-1))
		return CBN_STATUS_INVALID_ARGUMENT;

	if(!Vm::is_valid_user_range(address_space, (unsigned long) hint, length))
		return CBN_STATUS_INVALID_ARGUMENT;

	if(Vm::is_region_span_free_unlocked(address_space, hint, length))
	{
		/* If its free, map where we want it to, else fail */
		auto reg = vm_reserve_region(address_space, (unsigned long) hint, length);
		if(!reg)
			return CBN_STATUS_OUT_OF_MEMORY;

		reg->perms = cbn_mmap_to_vm_prots(prot);
		reg->off = off;
		out_region = reg;
		return CBN_STATUS_OK;
	}
	else
		return CBN_STATUS_OUT_OF_MEMORY;
		
}
cbn_status_t cbn_mmap_create_vmregion(process *target, void *hint, size_t length,
				   size_t off, long flags, long prot, vm_region *&out_region)
{
	auto address_space = &target->address_space;
	scoped_spinlock guard{&address_space->lock};
	cbn_status_t st = 0;

	bool fixed = flags & MAP_FILE_FIXED;
	
	length = page_align_up(length);

	if(fixed)
	{
		/* If it's fixed and the in place mapping fails, error out */
		if((st = cbn_mmap_in_place(address_space, hint, length, off,
			prot, out_region)) != CBN_STATUS_OK)
		{
			return st;
		}
	}

	if(hint != nullptr)
	{
		if((st = cbn_mmap_in_place(address_space, hint, length, off,
			prot, out_region)) == CBN_STATUS_OK)
		{
			return st;
		}
	}

	struct vm_region *region = Vm::AllocateRegionInternal(address_space,
					address_space->start,
					length);
	if(!region)
		return CBN_STATUS_OUT_OF_MEMORY;
	region->perms = cbn_mmap_to_vm_prots(prot);
	region->off = off;
	out_region = region;
	return CBN_STATUS_OK;
}

cbn_status_t sys_cbn_mmap(cbn_handle_t process_handle, cbn_handle_t vmo_handle, void *hint,
			     struct __cbn_mmap_packed_args *packed_args, long prot, void **result)
{
	using namespace vm;
	using namespace Vm;

	/* We need this struct because of the argument limit(6) */
	__cbn_mmap_packed_args kargs;
	if(copy_from_user(&kargs, packed_args, sizeof(kargs)) < 0)
		return CBN_STATUS_SEGFAULT;
	
	auto target_process = get_process_from_handle(process_handle);
	cbn_status_t st = CBN_STATUS_OK;

	if(!target_process)
		return CBN_STATUS_INVALID_HANDLE;
	
	vm_object *target_vmo = nullptr;

	if((st = cbn_mmap_get_vm_object_for_map(vmo_handle, kargs.flags, target_vmo)) != CBN_STATUS_OK)
		return CBN_STATUS_OK;

	vm_region *region = nullptr;
	if((st = cbn_mmap_create_vmregion(target_process, hint, kargs.length,
					 kargs.off, kargs.flags, prot, region)) != CBN_STATUS_OK)
	{
		target_vmo->unref();
		return st;
	}

	region->vmo = target_vmo;

	if(copy_to_user(result, &region->start, sizeof(void *)) < 0)
	{
		vm_destroy_region(&target_process->address_space, region);
		return CBN_STATUS_SEGFAULT;
	}

	return CBN_STATUS_OK;
}