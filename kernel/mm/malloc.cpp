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

#include <carbon/vm.h>
#include <carbon/lock.h>
#include <carbon/memory.h>
#include <carbon/page.h>
#include <carbon/heap.h>

static struct heap heap = {};
uintptr_t starting_address = 0;

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

extern "C"
void panic(char *);
void *expand_heap(size_t size)
{
	size_t nr_pages = (size >> PAGE_SHIFT) + 3;

	void *alloc_start = heap.brk;
	/*printf("hello???\n");
	printf("Expanding heap from %p to %lx\n", heap.brk, (unsigned long) alloc_start + nr_pages << PAGE_SHIFT);*/
	if(!map_pages(alloc_start, VM_PROT_WRITE, nr_pages))
		return NULL;

	heap.size += nr_pages << PAGE_SHIFT;

	return alloc_start;
}

void *do_brk_change(intptr_t inc)
{
	assert(heap.brk != NULL);
	void *old_brk = heap.brk;
	
	uintptr_t new_brk = (uintptr_t) heap.brk + inc;
	uintptr_t starting_address = (uintptr_t) heap.starting_address;
	
	if(new_brk >= starting_address + heap.size)
	{
		size_t size = new_brk - starting_address;

		void *ptr = expand_heap(size);
		if(!ptr)
			return errno = ENOMEM, (void *) -1;
	}

	heap.brk = (void*) new_brk;

	return old_brk;
}

extern "C" void *sbrk(intptr_t increment)
{
	return do_brk_change(increment);
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
