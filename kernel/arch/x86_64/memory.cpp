/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <carbon/memory.h>
#include <carbon/page.h>
#include <carbon/vm.h>

#define PML_EXTRACT_ADDRESS(n) (n & 0x0FFFFFFFFFFFF000)

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

struct x86_address_space
{
	PML *pml;
};

PML *boot_pml4;
struct x86_address_space boot_x86_address_space;

static PML pdptphysical_map __attribute__((aligned(4096)));
static PML pdphysical_map[512] __attribute__((aligned(4096)));

#define EARLY_BOOT_GDB_DELAY	\
volatile int __gdb_debug_counter = 0; \
while(__gdb_debug_counter != 1)

extern uintptr_t base_address;

void x86_setup_physical_mappings(void)
{
	/* Get the current PML4 and store it */
	__asm__ __volatile__("movq %%cr3, %%rax\t\nmovq %%rax, %0":"=r"(boot_pml4));

	boot_x86_address_space.pml = boot_pml4;
	uintptr_t virt = PHYS_BASE;
	assert(((virt >> 12) >> 18 & 0x1ff) == 0);

	uint64_t* entry = &boot_pml4->entries[(virt >> 12) >> 27 & 0x1ff];
	PML* pml3 = (PML*) &pdptphysical_map;

	memset(pml3, 0, sizeof(PML));
	*entry = make_pml4e(((uint64_t) pml3 - base_address), 0, 0, 0, 0, 1,
		1);
	entry = &pml3->entries[0];

	PML *pd = pdphysical_map;

	for(size_t i = 0; i < 512; i++)
	{
		*entry = make_pml3e(((unsigned long) pd - base_address), 0, 0,
			1, 0, 0, 0, 1, 1);
		
		for(size_t j = 0; j < 512; j++)
		{
			uintptr_t p = i * 512 * 0x200000 + j * 0x200000; 

			pd->entries[j] = p | 0x83;
		}
		pd++;
		entry++;
	}

	__native_tlb_invalidate_all();
}

void *__map_phys_to_virt(HANDLE addr, uintptr_t virt, uintptr_t phys,
unsigned long prot)
{
	struct x86_address_space *aspace = (struct x86_address_space *) addr;
 
	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];

	bool is_write = prot & VM_PROT_WRITE;
	bool is_user = prot & VM_PROT_USER;
	bool is_global = is_user;
	bool is_nx = !(prot & VM_PROT_EXEC);

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (virt >> 12) >> (i * 9) & 0x1ff;
	}

	PML *pml = (PML*)((uint64_t) aspace->pml + PHYS_BASE);
	
	for(unsigned int i = paging_levels; i != 1; i--)
	{
		uint64_t entry = pml->entries[indices[i - 1]];
		if(entry & 1)
		{
			void *page = (void*) PML_EXTRACT_ADDRESS(entry);
			pml = (PML *) phys_to_virt(page);
		}
		else
		{
			struct page *p = alloc_pages(1, 0);
			if(!p)
				return NULL;
			void *page = p->paddr;
			memset(phys_to_virt(page), 0, PAGE_SIZE);
			if(i == 3)
			{
				pml->entries[indices[i - 1]] =
				make_pml4e((uint64_t) page, 0, 0, 0, is_user,
					1, 1);
			}
			else
			{
				pml->entries[indices[i - 1]] =
				make_pml3e((uint64_t) page, 0, 0, 0, 0, 0,
					is_user, 1, 1);
			}
			pml = (PML *) phys_to_virt(page);
		}
	}
	
	pml->entries[indices[0]] = make_pml1e(phys, is_nx, 0,
		is_global, 0, 0, is_user, is_write, 1);
	return (void*) virt;
}

HANDLE get_x86_address_space_handle(void)
{
	return &boot_x86_address_space;
}

void *map_phys_to_virt(uintptr_t virt, uintptr_t phys, unsigned long prot)
{
	HANDLE addr = get_x86_address_space_handle();
	return __map_phys_to_virt(addr, virt, phys, prot);
}

void flush_tlb(void *addr, size_t nr_pages)
{
	uintptr_t i = (uintptr_t) addr;

	do
	{
		__native_tlb_invalidate_page((void *) i);
		i += PAGE_SIZE;
	} while(--nr_pages);
}

bool pml_is_empty(PML *pml)
{
	for(int i = 0; i < 512; i++)
	{
		if(pml->entries[i])
			return false;
	}
	return true;
}

void __unmap_page(struct x86_address_space *as, void *addr)
{
	struct x86_address_space *aspace = (struct x86_address_space *) as;
 
	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];
	unsigned long virt = (unsigned long) addr;

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (virt >> 12) >> (i * 9) & 0x1ff;
	}

	unsigned long entry = 0;

	PML *pml4 = (PML*) phys_to_virt(aspace->pml);

	entry = pml4->entries[indices[3]];

	if(!entry & 1)
		return;

	PML *pml3 = (PML*) phys_to_virt(PML_EXTRACT_ADDRESS(entry));
	if(!pml3)
		return;
	
	entry = pml3->entries[indices[2]];

	if(!entry & 1)
		return;

	PML *pml2 = (PML*) phys_to_virt(PML_EXTRACT_ADDRESS(entry));
	if(!pml2)
		return;

	entry = pml2->entries[indices[1]];

	if(!entry & 1)
		return;

	PML *pml1 = (PML*) phys_to_virt(PML_EXTRACT_ADDRESS(entry));
	if(!pml1)
		return;
	
	pml1->entries[indices[0]] = 0;

	/* Now that we've freed the destination page, work our way upwards to
	 * check if the paging structures are empty.
	 * If so, free them as well.
	*/

	if(pml_is_empty(pml1))
	{
		uintptr_t raw_address = PML_EXTRACT_ADDRESS(pml2->entries[indices[1]]);
		free_page(phys_to_page(raw_address));
		pml2->entries[indices[1]] = 0;
	}

	if(pml_is_empty(pml2))
	{
		uintptr_t raw_address = PML_EXTRACT_ADDRESS(pml3->entries[indices[2]]);
		free_page(phys_to_page(raw_address));
		pml3->entries[indices[2]] = 0;
	}

	if(pml_is_empty(pml3))
	{
		uintptr_t raw_address = PML_EXTRACT_ADDRESS(pml4->entries[indices[3]]);
		free_page(phys_to_page(raw_address));
		pml4->entries[indices[3]] = 0;
	}
}

void unmap_page_range(void *as, void *addr, size_t len)
{
	size_t nr_pgs = size_to_pages(len);
	unsigned long a = (unsigned long) addr;

	while(nr_pgs--)
	{
		/* TODO: Connect this up */
		__unmap_page(&boot_x86_address_space, (void *) a);
		a += PAGE_SIZE;
	}

	flush_tlb(addr, size_to_pages(len));
}