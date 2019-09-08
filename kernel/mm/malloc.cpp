/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <carbon/panic.h>
#include <carbon/vm.h>
#include <carbon/lock.h>
#include <carbon/memory.h>
#include <carbon/page.h>
#include <carbon/heap.h>

static struct heap heap = {};
uintptr_t starting_address = 0;
Spinlock heap_lock{};

extern "C"
struct heap *heap_get()
{
	return &heap;
}

void heap_set_start(uintptr_t heap_start)
{
	starting_address = heap_start;
	heap.starting_address = (void *) heap_start;
	heap.brk = heap.starting_address;
	heap.size = 0;
}

int kasan_alloc_shadow(unsigned long addr, size_t size, bool accessible);

void *expand_heap(size_t size)
{
	size_t nr_pages = (size >> PAGE_SHIFT) + 3;

	void *alloc_start = (void *) ((char *) heap.starting_address + heap.size);
#if 0
	printf("Expanding heap from %p to %lx\n", alloc_start, (unsigned long) alloc_start + (nr_pages << PAGE_SHIFT));
#endif
	if(!map_pages(alloc_start, VM_PROT_WRITE, nr_pages))
		return NULL;
	
	heap.size += nr_pages << PAGE_SHIFT;

	return alloc_start;
}

void *do_brk_change(intptr_t inc)
{
	assert(heap.brk != NULL);
	scoped_spinlock guard{&heap_lock};

	void *old_brk = heap.brk;

	uintptr_t new_brk = (uintptr_t) heap.brk + inc;
	uintptr_t starting_address = (uintptr_t) heap.starting_address;
	unsigned long heap_limit = starting_address + heap.size;
	if(new_brk >= heap_limit)
	{
		size_t size = new_brk - heap_limit;

		void *ptr = expand_heap(size);
		if(!ptr)
			return errno = ENOMEM, (void *) -1;
	}

	heap.brk = (void*) new_brk;

	return old_brk;
}

extern "C" void *sbrk(intptr_t increment)
{
	void *ret = do_brk_change(increment);
	return ret;
}

void *zalloc(size_t size)
{
	return calloc(1, size);
}

void malloc_reserve_memory_space(void)
{
	assert(vm_reserve_region(kernel_as, (unsigned long) starting_address,
		0x10000000000) != NULL);
}
