/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/panic.h>

#include <libdict/dict.h>

#define KADDR_SPACE_SIZE	0x800000000000
#define KADDR_START		0xffff800000000000

struct address_space kernel_address_space = {0};

int vm_cmp(const void* k1, const void* k2)
{
	if(k1 == k2)
		return 0;

        return (unsigned long) k1 < (unsigned long) k2 ? -1 : 1; 
}

struct vm_region *vm_reserve_region(struct address_space *as,
				    unsigned long start, size_t size)
{
	struct vm_region *region = malloc(sizeof(*region));
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

	if(min != as->start)
		node_valid = rb_itor_search_ge(it, (const void *) min);
	else
	{
		node_valid = rb_itor_first(it);
		if(!node_valid)
			goto done;
	}
	

	/* Check if there's a gap between the first node
	 * and the start of the address space
	*/

	struct vm_region *f = *rb_itor_datum(it);

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
		struct vm_region *f = *rb_itor_datum(it);
		last_end = f->start + f->size;
 
		node_valid = rb_itor_next(it);
		if(!node_valid)
			break;

		struct vm_region *vm = *rb_itor_datum(it);

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