/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <carbon/panic.h>
#include <carbon/vm.h>
#include <carbon/memory.h>
#include <carbon/page.h>
#include <carbon/heap.h>

#define KADDR_SPACE_SIZE	0x800000000000
#define KADDR_START		0xffff800000000000

#define __alias(symbol)                 __attribute__((__alias__(#symbol)))

#define KASAN_SHIFT		3
#define KASAN_N_MASK		0x7
#define KASAN_ACCESSIBLE	0x0
#define KASAN_REDZONE		-1
#define KASAN_FREED		-2

bool kasan_is_init = false;

#define arch_high_half		KADDR_START
#define arch_kasan_off		0x100000000000

#define ADDR_SPACE_SIZE		(KADDR_SPACE_SIZE/8)

constexpr unsigned long kasan_space = arch_high_half + arch_kasan_off;

inline char *kasan_get_ptr(unsigned long addr)
{
	return (char *) (kasan_space + ((addr - arch_high_half) >> KASAN_SHIFT));
}

void kasan_fail(unsigned long addr, size_t size, bool write)
{
	/* Disable kasan so we can panic */
	kasan_is_init = false;
	while(1);
	printf("Kasan: panic at %lx, %s of size %lu\n", addr, write ? "write" : "read", size);
	panic("crap");
}

#define KASAN_MISALIGNMENT(x)	(x & (8-1))

void kasan_check_memory_fast(unsigned long addr, size_t size, bool write)
{
	char b = *kasan_get_ptr(addr);

	unsigned int n = addr & KASAN_N_MASK;

	if(b == KASAN_ACCESSIBLE)
		return;
	
	unsigned int first_n_accessible = b;

	if(n >= first_n_accessible)
		kasan_fail(addr, size, write);
}

void kasan_check_memory(unsigned long addr, size_t size, bool write)
{
	if(!kasan_is_init)
		return;
	size_t n = KASAN_MISALIGNMENT(addr);

	if(n + size <= 8)
		kasan_check_memory_fast(addr, size, write);

	while(size != 0)
	{
		char b = *kasan_get_ptr(addr);

		unsigned int n = addr & KASAN_N_MASK;

		if(b == KASAN_ACCESSIBLE)
		{
			addr += 8 - n;
			size -= n;
		}
		else
		{
			unsigned int first_n_accessible = b;

			if(n >= first_n_accessible)
				kasan_fail(addr, size, write);
		}
	}
}

#define KASAN_LOAD(size) \
extern "C"					\
void __asan_load##size(unsigned long addr) 	\
{						\
	kasan_check_memory(addr, size, false);	\
}						\
extern "C"			\
void __asan_load##size##_noabort(unsigned long addr) __alias(__asan_load##size);

#define KASAN_STORE(size) \
extern "C" \
void __asan_store##size(unsigned long addr) 	\
{						\
	kasan_check_memory(addr, size, true);	\
}						\
extern "C" \
void __asan_store##size##_noabort(unsigned long addr) __alias(__asan_store##size);


KASAN_LOAD(1);
KASAN_LOAD(2);
KASAN_LOAD(4);
KASAN_LOAD(8);
KASAN_LOAD(16);

KASAN_STORE(1);
KASAN_STORE(2);
KASAN_STORE(4);
KASAN_STORE(8);
KASAN_STORE(16);

extern "C"
void __asan_loadN(unsigned long addr, size_t size)
{
	kasan_check_memory(addr, size, false);
}

extern "C"
void __asan_loadN_noabort(unsigned long addr, size_t size) __alias(__asan_loadN);

extern "C"
void __asan_storeN(unsigned long addr, size_t size)
{
	kasan_check_memory(addr, size, true);
}

extern "C"
void __asan_storeN_noabort(unsigned long addr, size_t size) __alias(__asan_storeN);

extern "C"
void __asan_handle_no_return(void) {}

extern "C"
void __asan_before_dynamic_init(void) {}

extern "C"
void __asan_after_dynamic_init(void) {}

void asan_init(void)
{
	void *k = (void *) kasan_space;

	Vm::ForEveryRegion(&kernel_address_space, [] (struct vm_region *region) -> bool
	{
		printf("Region base: %lx\n", region->start);

		unsigned long region_start = region->start;
		unsigned long region_end = region->start + region->size;

		
		return true;
	});

	return;
	kasan_is_init = true;
}

#define EARLY_BOOT_GDB_DELAY	\
volatile int __gdb_debug_counter = 0; \
while(__gdb_debug_counter != 1)


int kasan_alloc_shadow(unsigned long addr, size_t size, bool accessible)
{
	printf("Kasan alloc shadow: %lx - %lx\n", addr, addr + size);

	auto kasan_start = (unsigned long) kasan_get_ptr(addr);
	auto kasan_end = (unsigned long) kasan_get_ptr(addr + size);

	auto actual_start = kasan_start, actual_end = kasan_end;

	/* Align the boundaries */

	kasan_start &= ~(PAGE_SIZE - 1);

	if(kasan_end & (PAGE_SIZE - 1))
	{
		kasan_end += PAGE_SIZE - (kasan_end & (PAGE_SIZE - 1));
	}

	/*printf("Kasan start: %lx\n", kasan_start);
	printf("Kasan end: %lx\n", kasan_end);
	printf("Actual start: %lx\nActual end: %lx\n", actual_start, actual_end);*/

	assert(map_pages((void *) kasan_start, PAGE_PROT_GLOBAL | PAGE_PROT_WRITE | PAGE_PROT_READ,
		(kasan_end - kasan_start) >> PAGE_SHIFT) != nullptr);
	/* Mask excess bytes as redzones */
	memset((void *) kasan_start, KASAN_REDZONE, actual_start - kasan_start);
	memset((void *) actual_end, KASAN_REDZONE, kasan_end - actual_end);

	if(!accessible)
	{
		memset((void *) actual_start, (unsigned char) KASAN_REDZONE, size);
	}

	return 0;
}

extern "C"
void kasan_set_state(unsigned long *ptr, size_t size, int state)
{
	/* State: 0 = Accessible, -1 = redzone, 1 = freed */
	while(size)
	{
		size_t n = KASAN_MISALIGNMENT((unsigned long) ptr);
		size_t to_set = size < 8 - n ? size : 8 - n;
	}
}