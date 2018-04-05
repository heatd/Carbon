/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>

#include <carbon/bootprotocol.h>

#define RELOCATE_R_X86_64_64(S, A) (S + A)
#define RELOCATE_R_X86_64_32(S, A) (S + A)
#define RELOCATE_R_X86_64_16(S, A) (S + A)
#define RELOCATE_R_X86_64_8(S, A) (S + A)
#define RELOCATE_R_X86_64_32S(S, A) (S + A)
#define RELOCATE_R_X86_64_PC32(S, A, P) (S + A - P)
#define RELOCATE_R_X86_64_RELATIVE(B, A) (B + A)

#define R_X86_64_PLT32		4
#define R_X86_64_NONE	0
#define R_X86_64_64	1
#define R_X86_64_PC32	2
#define R_X86_64_32	10
#define R_X86_64_32S	11

void do_r_x86_64_none(uintptr_t symbol, uintptr_t addend, uintptr_t pc)
{}

void do_r_x86_64_64(uintptr_t symbol, uintptr_t addend, uintptr_t pc)
{
	uintptr_t *p = (uintptr_t *) pc;
	*p = RELOCATE_R_X86_64_64(symbol, addend);
}

void do_r_x86_64_32S(uintptr_t symbol, uintptr_t addend, uintptr_t pc)
{
	int32_t *p = (int32_t *) pc;
	*p = RELOCATE_R_X86_64_32S(symbol, addend);
}

void do_r_x86_64_32(uintptr_t symbol, uintptr_t addend, uintptr_t pc)
{
	uint32_t *p = (uint32_t *) pc;
	*p = RELOCATE_R_X86_64_32(symbol, addend);
}

void do_r_x86_64_pc32(uintptr_t symbol, uintptr_t addend, uintptr_t pc)
{
	/* PC32 relocations don't need to be handled as we're not linking anything */
	if(1)
		return;
	uint32_t *p = (uint32_t *) pc;

	*p = RELOCATE_R_X86_64_PC32(symbol, addend, pc);
}

void do_r_x86_64_plt32(uintptr_t symbol, uintptr_t addend, uintptr_t pc)
{
	/* PLT32 relocations don't need to be handled as we're not linking anything */
	if(1)
		return;
	uint32_t *p = (uint32_t *) pc;

	*p = RELOCATE_R_X86_64_PC32(symbol, addend, pc);
}

extern uint64_t pml4[4096];
extern uint64_t pdpt[4096];
extern uint64_t pdpt2[4096];
extern uint64_t pdlower[4096];
extern uint64_t pdhigher[4096];

void remap_kernel(uintptr_t base_address)
{
	const unsigned int paging_levels = 4;
	unsigned int indices[paging_levels];

	for(unsigned int i = 0; i < paging_levels; i++)
	{
		indices[i] = (base_address >> 12) >> (i * 9) & 0x1ff;
	}
	(void) indices;
	uint64_t *pml = pml4;
	for(unsigned int i = paging_levels; i != 1; i--)
	{
		if(i == 4)
		{
			pml[indices[i - 1]] = (uint64_t) pdpt | 0x3;
			pml[0] = (uint64_t) pdpt2 | 0x3;
			
			pml = pdpt;
		}
		else if(i == 3)
		{
			pml[indices[i - 1]] = (uint64_t) pdhigher | 0x3;
			pdpt2[0] = (uint64_t) pdlower | 0x3;

			pml = pdhigher;
		}
		else if(i == 2)
		{
			unsigned int base_index = indices[i - 1];
			pml[base_index] = 0x0 | 0x83;
			pml[base_index + 1] = 0x200000 | 0x83;
			pml[base_index + 2] = 0x400000 | 0x83;

			pdlower[0] = 0x0 | 0x83;
			pdlower[1] = 0x200000 | 0x83;
			pdlower[2] = 0x400000 | 0x83;
		}
	}

}

void relocate_kernel(struct boot_info *info, uintptr_t base_address)
{
	remap_kernel(base_address);
	
	struct relocation *r;
	void (*reloc_jmp_tab[])(uintptr_t symbol, uintptr_t addend, uintptr_t pc) =
	{
		do_r_x86_64_none,
		[R_X86_64_64] = do_r_x86_64_64,
		[R_X86_64_32S] = do_r_x86_64_32S,
		[R_X86_64_32] = do_r_x86_64_32,
		[R_X86_64_PC32] = do_r_x86_64_pc32,
		[R_X86_64_PLT32] = do_r_x86_64_plt32
	};

	/* Use a jump table in order to avoid relocations(so we don't patch ourselves
	 to invalid addresses) */
	for(r = info->relocations; r != NULL; r = r->next)
	{
		r->symbol_value += base_address;
		reloc_jmp_tab[r->type](r->symbol_value, r->addend, (uintptr_t) r->address);
	}
}
