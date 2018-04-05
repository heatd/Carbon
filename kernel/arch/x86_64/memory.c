/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <carbon/memory.h>

typedef struct
{
	uint64_t entries[512];
} PML;

static inline void __native_tlb_invalidate_page(void *addr)
{
	__asm__ __volatile__("invlpg %0"::"m"(addr));
}

static inline void __native_tlb_invalidate_all()
{
	__asm__ __volatile__("mov %cr3, %rax; mov %rax, %cr3");
}

static inline uint64_t make_pml4e(uint64_t base,
				  uint64_t avl,
				  uint64_t pcd,
				  uint64_t pwt,
				  uint64_t us,
				  uint64_t rw,
				  uint64_t p)
{
	return (uint64_t)( \
  		(base) | \
  		(avl << 9) | \
  		(pcd << 4) | \
  		(pwt << 3) | \
  		(us << 2) | \
  		(rw << 1) | \
  		p);
}

static inline uint64_t make_pml3e(uint64_t base,
				  uint64_t nx,
				  uint64_t avl,
				  uint64_t glbl,
				  uint64_t pcd,
				  uint64_t pwt,
				  uint64_t us,
				  uint64_t rw,
				  uint64_t p)
{
	return (uint64_t)( \
  		(base) | \
  		(nx << 63) | \
  		(avl << 9) | \
  		(glbl << 8) | \
  		(pcd << 4) | \
  		(pwt << 3) | \
  		(us << 2) | \
  		(rw << 1) | \
  		p);
}

static inline uint64_t make_pml2e(uint64_t base,
				  uint64_t nx,
				  uint64_t avl,
				  uint64_t glbl,
				  uint64_t pcd,
				  uint64_t pwt,
				  uint64_t us,
				  uint64_t rw,
				  uint64_t p)
{
	return (uint64_t)( \
  		(base) | \
  		(nx << 63) | \
  		(avl << 9) | \
  		(glbl << 8) | \
  		(pcd << 4) | \
  		(pwt << 3) | \
  		(us << 2) | \
  		(rw << 1) | \
  		p);
}

static inline uint64_t make_pml1e(uint64_t base,
				  uint64_t nx,
				  uint64_t avl,
				  uint64_t glbl,
				  uint64_t pcd,
				  uint64_t pwt,
				  uint64_t us,
				  uint64_t rw,
				  uint64_t p)
{
	return (uint64_t)( \
  		(base) | \
  		(nx << 63) | \
  		(avl << 9) | \
  		(glbl << 8) | \
  		(pcd << 4) | \
  		(pwt << 3) | \
  		(us << 2) | \
  		(rw << 1) | \
  		p);
}

PML *boot_pml4;
static PML pdptphysical_map __attribute__((aligned(4096)));
static PML pdphysical_map __attribute__((aligned(4096)));

#define EARLY_BOOT_GDB_DELAY	\
volatile int __gdb_debug_counter = 0; \
while(__gdb_debug_counter != 1)

extern uintptr_t base_address;

void x86_setup_physical_mappings(void)
{
	/* Get the current PML4 and store it */
	__asm__ __volatile__("movq %%cr3, %%rax\t\nmovq %%rax, %0":"=r"(boot_pml4));

	/* Bootstrap the first 1GB */
	uintptr_t virt = PHYS_BASE;
	uint64_t* entry = &boot_pml4->entries[(virt >> 12) >> 27 & 0x1ff];
	PML* pml3 = (PML*) &pdptphysical_map;

	memset(pml3, 0, sizeof(PML));
	*entry = make_pml4e(((uint64_t) pml3 - base_address), 0, 0, 0, 0, 1, 1);
	entry = &pml3->entries[(virt >> 12) >> 18 & 0x1ff];
	*entry = make_pml3e(((uint64_t) &pdphysical_map - base_address), 0, 0, 1, 0, 0, 0, 1, 1);
	
	for(size_t j = 0; j < 512; j++)
	{
		uintptr_t p = j * 0x200000; 
		
		pdphysical_map.entries[j] = p | 0x83;
	}

	__native_tlb_invalidate_all();
}
