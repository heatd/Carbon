/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdlib.h>
#include <string.h> 
#include <assert.h>

#include <carbon/percpu.h>

#include <carbon/x86/tss.h>

namespace Tss
{

typedef struct tss_entry
{
	uint32_t reserved0;
	uint64_t stack0; /* This is not naturally aligned, so packed is needed. */
	uint64_t stack1;
	uint64_t stack2;
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

PER_CPU_VAR(tss_entry_t *tss) = nullptr;

void SetKernelStack(unsigned long stack)
{
	auto this_tss = get_per_cpu(tss);
	this_tss->stack0 = stack;
	this_tss->ist[0] = stack; 
}

extern "C" void tss_flush();

void InitPercpu(uint64_t *gdt)
{
	tss_entry_t *new_tss = (tss_entry_t *) malloc(sizeof(tss_entry_t));
	assert(new_tss != nullptr);
	memset(new_tss, 0, sizeof(tss_entry_t));

	uint8_t *tss_gdtb = (uint8_t*) &gdt[7];
	uint16_t *tss_gdtw = (uint16_t*) &gdt[7];
	uint32_t *tss_gdtd = (uint32_t*) &gdt[7];
	tss_gdtw[1] = (uintptr_t) new_tss & 0xFFFF;
	tss_gdtb[4] = ((uintptr_t) new_tss >> 16) & 0xFF;
	tss_gdtb[6] = ((uintptr_t) new_tss >> 24) & 0xFF;
	tss_gdtb[7] = ((uintptr_t) new_tss >> 24) & 0xFF;
	tss_gdtd[2] = ((uintptr_t) new_tss >> 32);
	tss_flush();

	write_per_cpu(tss, new_tss);
}

}