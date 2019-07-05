/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <carbon/percpu.h>
#include <carbon/x86/msr.h>

extern unsigned char __percpu_start;
extern unsigned char __percpu_end;

namespace Percpu
{

PER_CPU_VAR(unsigned long __cpu_base) = 0;

void Init()
{
	size_t percpu_size = (unsigned long) &__percpu_end - (unsigned long) &__percpu_start;
	printf("Percpu size: %lu\n", percpu_size);

	void *buffer = zalloc(percpu_size);
	assert(buffer != nullptr);

	printf("buffer: %p\n", buffer);

	wrmsr(GS_BASE_MSR, (unsigned long) buffer);

	write_per_cpu(__cpu_base, (unsigned long) buffer);

	assert(get_per_cpu(__cpu_base) == (unsigned long) buffer);
}

}