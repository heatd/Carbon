/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <string.h>

#include <carbon/memory.h>
#include <carbon/bootprotocol.h>

#define RELOCATE_R_X86_64_64(S, A) (S + A)
#define RELOCATE_R_X86_64_32(S, A) (S + A)
#define RELOCATE_R_X86_64_16(S, A) (S + A)
#define RELOCATE_R_X86_64_8(S, A) (S + A)
#define RELOCATE_R_X86_64_32S(S, A) (S + A)
#define RELOCATE_R_X86_64_PC32(S, A, P) (S + A - P)
#define RELOCATE_R_X86_64_RELATIVE(B, A) (B + A)
#define DEFAULT_VIRT_BASE		0xffffffff80000000

#define R_X86_64_PLT32		4
#define R_X86_64_NONE	0
#define R_X86_64_64	1
#define R_X86_64_PC32	2
#define R_X86_64_32	10
#define R_X86_64_32S	11

#define SECTION_BOOT	__attribute__((section(".boot")))
#define SECTION_BOOTDATA	__attribute__((section(".boot_data")))

SECTION_BOOTDATA
uintptr_t kernel_base = 0;

SECTION_BOOT
void do_r_x86_64_none(uintptr_t symbol, uintptr_t addend, uintptr_t pc, void *p)
{}

SECTION_BOOT
void do_r_x86_64_64(uintptr_t symbol, uintptr_t addend, uintptr_t pc, void *__p)
{
	uintptr_t *p = (uintptr_t *) __p;
	*p = RELOCATE_R_X86_64_64(symbol, addend);
}

SECTION_BOOT
void do_r_x86_64_32S(uintptr_t symbol, uintptr_t addend, uintptr_t pc, void *__p)
{
	int32_t *p = (int32_t *) __p;
	*p = RELOCATE_R_X86_64_32S(symbol, addend);
}

SECTION_BOOT
void do_r_x86_64_32(uintptr_t symbol, uintptr_t addend, uintptr_t pc, void *__p)
{
	uint32_t *p = (uint32_t *) __p;
	*p = RELOCATE_R_X86_64_32(symbol, addend);
}

SECTION_BOOT
void do_r_x86_64_pc32(uintptr_t symbol, uintptr_t addend, uintptr_t pc, void *__p)
{
	uint32_t *p = (uint32_t *) __p;

	*p = RELOCATE_R_X86_64_PC32(symbol, addend, pc);
}

extern uint64_t pml4[4096];
extern uint64_t pdpt[4096];
extern uint64_t pdpt2[4096];
extern uint64_t pdlower[4096];
extern uint64_t pdhigher[4096];

SECTION_BOOT
void remap_kernel(uintptr_t base_address)
{
	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (base_address >> 12) >> (i * 9) & 0x1ff;
	}

	uint64_t *__pdpt2 = (uint64_t *) &pdpt2;
	uint64_t *__pdlower = (uint64_t *) &pdlower;

	uint64_t *pml = pml4;
	for(unsigned int i = paging_levels; i != 1; i--)
	{
		if(i == 4)
		{
			memset(pml, 0, PAGE_SIZE);
			pml[indices[i - 1]] = (uint64_t) pdpt | 0x3;
			pml[0] = (uint64_t) pdpt2 | 0x3;

			pml = pdpt;
		}
		else if(i == 3)
		{
			memset(pml, 0, PAGE_SIZE);
			memset(__pdpt2, 0, PAGE_SIZE);
			pml[indices[i - 1]] = (uint64_t) pdhigher | 0x3;
			__pdpt2[0] = (uint64_t) pdlower | 0x3;

			pml = pdhigher;
		}
		else if(i == 2)
		{
			unsigned int base_index = indices[i - 1];
			memset(pml, 0, PAGE_SIZE);
			memset(__pdlower, 0, PAGE_SIZE);
			pml[base_index] = 0x0 | 0x83;
			pml[base_index + 1] = 0x200000 | 0x83;
			pml[base_index + 2] = 0x400000 | 0x83;

			__pdlower[0] = 0x0 | 0x83;
			__pdlower[1] = 0x200000 | 0x83;
			__pdlower[2] = 0x400000 | 0x83;
		}
	}
}

SECTION_BOOT
void relocate_kernel(struct boot_info *info, uintptr_t base_address)
{
	remap_kernel(base_address);
#ifdef CONFIG_RELOCATABLE
	kernel_base = base_address;

	struct relocation *r;
	void (*reloc_jmp_tab[])(uintptr_t symbol, uintptr_t addend, uintptr_t pc, void *p) =
	{
		do_r_x86_64_none,
		[R_X86_64_64] = do_r_x86_64_64,
		[R_X86_64_32S] = do_r_x86_64_32S,
		[R_X86_64_32] = do_r_x86_64_32,
		[R_X86_64_PC32] = do_r_x86_64_pc32,
		[R_X86_64_PLT32] = do_r_x86_64_pc32
	};

	/* Use a jump table in order to avoid relocations(so we don't patch ourselves
	 to invalid addresses) */
	for(r = info->relocations; r != NULL; r = r->next)
	{
		/* The symbol value we get is actually phys_addr + DEFAULT_VIRT_BASE
		 * so to get the relocated symbol, we sub DEFAULT_VIRT_BASE and add base_address */
		bool sym_is_higher = r->symbol_value > DEFAULT_VIRT_BASE;
		if(sym_is_higher)
			r->symbol_value = r->symbol_value - DEFAULT_VIRT_BASE + base_address;

		/* if(r->symbol_value < DEFAULT_VIRT_BASE && r->symbol_value)
			__asm__ __volatile__("hlt");*/

		unsigned long index = r->type;
		if(index != R_X86_64_64 && index != R_X86_64_32S && index != R_X86_64_32 &&
		   index != R_X86_64_PC32 && index != R_X86_64_PLT32)
		   	__asm__ __volatile__("hlt");
		if(index >= sizeof(reloc_jmp_tab) / 8)
			__asm__ __volatile__("hlt");
		unsigned long addr = (unsigned long) r->address;
		bool is_higher_half = addr > DEFAULT_VIRT_BASE;
		unsigned long final_pc = is_higher_half ? addr - DEFAULT_VIRT_BASE + base_address : addr;
		unsigned long pointer = is_higher_half ? addr - DEFAULT_VIRT_BASE : addr;
		reloc_jmp_tab[index](r->symbol_value, r->addend, final_pc, (void *) pointer);
	}
#endif

}
