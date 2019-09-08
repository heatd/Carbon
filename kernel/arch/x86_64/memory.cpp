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
#include <carbon/x86/cpu.h>

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

PML *boot_pml4;

static PML pdptphysical_map __attribute__((aligned(4096)));
static PML pdphysical_map __attribute__((aligned(4096)));
static PML new_pml3;
static PML placement_mappings_page_dir __attribute__((aligned(4096)));
static PML placement_mappings_page_table __attribute__((aligned(4096)));

unsigned long placement_mappings_start = 0xffffffffffc00000;

#define EARLY_BOOT_GDB_DELAY	\
volatile int __gdb_debug_counter = 0; \
while(__gdb_debug_counter != 1)

extern uintptr_t base_address;


void *x86_placement_map(unsigned long _phys)
{
	//printf("_phys: %lx\n", _phys);
	unsigned long phys = _phys & ~(PAGE_SIZE - 1);
	//printf("phys: %lx\n", phys);

	/* Map two pages so memory that spans both pages can get accessed */
	map_phys_to_virt(placement_mappings_start, phys, VM_PROT_WRITE);
	map_phys_to_virt(placement_mappings_start + PAGE_SIZE, phys + PAGE_SIZE,
						VM_PROT_WRITE);
	__native_tlb_invalidate_page((void *) placement_mappings_start);
	__native_tlb_invalidate_page((void *) (placement_mappings_start + PAGE_SIZE));
	return (void *) (placement_mappings_start + (_phys - phys));
}

void x86_setup_placement_mappings(void)
{
	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];
	const unsigned long virt = placement_mappings_start;

	PML *pml = boot_pml4;

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (virt >> 12) >> (i * 9) & 0x1ff;
	}

	for(unsigned int i = paging_levels; i != 1; i--)
	{
		uint64_t entry = pml->entries[indices[i - 1]];
		if(entry & 1)
		{
			void *page = (void*) PML_EXTRACT_ADDRESS(entry);
			pml = (PML*) page;
		}
		else
		{
			unsigned long page = 0;
			if(i == 3)
			{
				page = ((unsigned long) &placement_mappings_page_dir - base_address);
			}
			else if(i == 2)
			{
				page = ((unsigned long) &placement_mappings_page_table - base_address);
			}
			else
			{
				/* We only handle non-present page tables for PML1 and 2 */
				__asm__ __volatile__("cli; hlt");
			}
		
			pml->entries[indices[i - 1]] = make_pml3e(page, 0, 0, 1, 0, 0, 0, 1, 1);

			pml = (PML *) page;
		}
	}
}

void x86_setup_early_physical_mappings(void)
{
	/* Get the current PML and store it */
	__asm__ __volatile__("movq %%cr3, %%rax\t\nmovq %%rax, %0":"=r"(boot_pml4));

	kernel_address_space.arch_priv = (PML *) boot_pml4;
	uintptr_t virt = PHYS_BASE;
	assert(((virt >> 12) >> 18 & 0x1ff) == 0);

	uint64_t* entry = &boot_pml4->entries[(virt >> 12) >> 27 & 0x1ff];
	PML* pml3 = (PML*) &pdptphysical_map;

	memset(pml3, 0, sizeof(PML));
	*entry = make_pml4e(((uint64_t) pml3 - base_address), 0, 0, 0, 0, 1,
		1);
	entry = &pml3->entries[0];

	PML *pd = &pdphysical_map;


	*entry = make_pml3e(((unsigned long) pd - base_address), 0, 0,
		1, 0, 0, 0, 1, 1);
		
	for(size_t j = 0; j < 512; j++)
	{
		uintptr_t p = j * 0x200000; 

		pd->entries[j] = p | 0x83 | (1UL << 63);
	}

	x86_setup_placement_mappings();

	__native_tlb_invalidate_all();
}

void *efi_allocate_early_boot_mem(size_t size);

void x86_setup_physical_mappings()
{
	const unsigned long virt = PHYS_BASE;

	bool is_huge_supported = x86::Cpu::HasCap(X86_FEATURE_PDPE1GB);

	PML *pml3 = (PML*) &pdptphysical_map;
	memset(&new_pml3, 0, sizeof(PML));

	if(is_huge_supported)
	{
		for(size_t i = 0; i < 512; i++)
		{
			auto entry = &pml3->entries[i];
			*entry = make_pml3e(i * 0x40000000, 1, 0, 1, 0, 0, 0, 1, 1);
			*entry |= (1 << 7);
			__native_tlb_invalidate_page((void*)(virt + i * 0x40000000));
		}
	}
	else
	{
		auto entry = &new_pml3.entries[0];
		for(size_t i = 0; i < 512; i++)
		{
			void *ptr = efi_allocate_early_boot_mem(PAGE_SIZE);

			assert(ptr != nullptr);

			*entry = make_pml3e(((unsigned long) ptr), 1, 0,
				1, 0, 0, 0, 1, 1);

			PML *pd = (PML*) phys_to_virt(ptr);

			for(size_t j = 0; j < 512; j++)
			{
				uintptr_t p = i * 512 * 0x200000 + j * 0x200000;

				pd->entries[j] = p | 0x83 | (1UL << 63);
			}

			entry++;
		}

		memcpy(pml3, &new_pml3, sizeof(PML));

		__native_tlb_invalidate_all();
	}
}

void *__map_phys_to_virt(void *addr, uintptr_t virt, uintptr_t phys,
	unsigned long prot)
{
	PML *pml4 = (PML *) addr;
	assert(pml4 != nullptr);

	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];

	bool is_write = prot & VM_PROT_WRITE;
	bool is_user = prot & VM_PROT_USER;
	bool is_global = !is_user;
	bool is_nx = !(prot & VM_PROT_EXEC);

	if(0 && !is_nx && is_write)
	{
		printf("map_phys_to_virt: Error: mapping of %lx"
		" at %lx violates W^X\n", phys, virt);
		return nullptr;
	}

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (virt >> 12) >> (i * 9) & 0x1ff;
	}

	PML *pml = (PML*)(phys_to_virt(pml4));
	
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

void *map_phys_to_virt(uintptr_t virt, uintptr_t phys, unsigned long prot)
{
	auto addr_space = Vm::get_current_address_space();
	return __map_phys_to_virt(addr_space->arch_priv, virt, phys, prot);
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

void __unmap_page(PML *__pml4, void *addr)
{ 
	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];
	unsigned long virt = (unsigned long) addr;

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (virt >> 12) >> (i * 9) & 0x1ff;
	}

	unsigned long entry = 0;

	PML *pml4 = (PML*) phys_to_virt(__pml4);

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
		__unmap_page(NULL, (void *) a);
		a += PAGE_SIZE;
	}

	flush_tlb(addr, size_to_pages(len));
}

extern char _text_start;
extern char _text_end;
extern char _data_start;
extern char _data_end;
extern char _vdso_sect_start;
extern char _vdso_sect_end;
extern char VIRT_BASE;
extern struct address_space kernel_address_space;

void map_phys_to_virt_pgs(unsigned long start, unsigned long pstart, size_t size, unsigned long prot)
{
	size_t pages = size >> PAGE_SHIFT;

	for(size_t i = 0; i < pages; i++)
	{
		map_phys_to_virt(start, pstart, prot);
		start += PAGE_SIZE;
		pstart += PAGE_SIZE;
	}
}

void paging_protect_kernel(void)
{
	PML *original_pml = boot_pml4;
	PML *pml;
	struct page *pa = alloc_pages(1, 0);
	assert(pa != nullptr);
	pml = (PML *) pa->paddr;
	boot_pml4 = pml;

	uintptr_t text_start = (uintptr_t) &_text_start;
	uintptr_t data_start = (uintptr_t) &_data_start;
	uintptr_t vdso_start = (uintptr_t) &_vdso_sect_start;

	memcpy((PML*)((uintptr_t) pml + PHYS_BASE), (PML*)((uintptr_t) original_pml + PHYS_BASE),
		sizeof(PML));
	PML *p = (PML*)((uintptr_t) pml + PHYS_BASE);
	p->entries[511] = 0UL;
	p->entries[0] = 0UL;

	kernel_address_space.arch_priv = pml;
	size_t size = (uintptr_t) &_text_end - text_start;
	map_phys_to_virt_pgs(text_start, (text_start - base_address),
		size, VM_PROT_EXEC);

	size = (uintptr_t) &_data_end - data_start;
	map_phys_to_virt_pgs(data_start, (data_start - base_address),
		size, VM_PROT_WRITE);
	
	size = (uintptr_t) &_vdso_sect_end - vdso_start;
	map_phys_to_virt_pgs(vdso_start, (vdso_start - base_address),
		size, VM_PROT_WRITE);

	__asm__ __volatile__("movq %0, %%cr3" :: "r"(pml));
}

#define PML_KERNEL_HALF_START		256
void *vm_create_page_tables()
{
	struct page *pml4_page = alloc_pages(1, 0);
	if(!pml4_page)
		return nullptr;
	PML *p = (PML*) phys_to_virt(pml4_page->paddr);
	memcpy(&p->entries[PML_KERNEL_HALF_START],
		phys_to_virt(&boot_pml4->entries[PML_KERNEL_HALF_START]),
		256 * sizeof(uint64_t));

	return pml4_page->paddr;  
}