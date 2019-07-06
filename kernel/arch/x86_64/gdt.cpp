/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdlib.h>
#include <stdio.h>	/* TODO: REMOVE */
#include <string.h>
#include <assert.h>

#include <carbon/panic.h>
#include <carbon/memory.h>
#include <carbon/x86/gdt.h>
#include <carbon/x86/tss.h>

#define GDT_SIZE 72

extern "C" Gdt::Gdtr gdtr;
extern "C" void gdt_flush(Gdt::Gdtr *gdtr);

void Gdt::InitPercpu(void)
{
	/* Create another copy of the gdt */
	uint64_t *gdt = (uint64_t *) malloc(GDT_SIZE);
	assert(gdt != nullptr);

	/* Create a gdtr */
	Gdtr *__gdtr = (Gdtr *) malloc(sizeof(Gdtr));
	if(!__gdtr)
	{
		free(gdt);
		panic("Out of memory while allocating a percpu GDT\n");
	}
	
	Gdtr *g = (Gdtr *) &gdtr;
	/* Copy the gdt */
	memcpy(gdt, (const void*) g->ptr, GDT_SIZE);

	/* Setup the GDTR */
	__gdtr->size = GDT_SIZE - 1;
	__gdtr->ptr = (uint64_t) gdt;

	/* Flush the GDT */
	gdt_flush(__gdtr);

	Tss::InitPercpu(gdt);
}