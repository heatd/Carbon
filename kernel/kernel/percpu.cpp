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

/* Define errno somewhere */
PER_CPU_VAR(int __true_errno) = 0;

PER_CPU_VAR(unsigned long __cpu_base) = 0;

extern "C" int *__errno_location()
{
	return get_per_cpu_ptr(__true_errno);
}

namespace Percpu
{

bool percpu_init = false;
unsigned long *percpu_bases = nullptr;
unsigned long nr_bases = 0;

void AddPercpu(unsigned long base)
{
	nr_bases++;
	percpu_bases = (unsigned long *) realloc((unsigned long *) percpu_bases,
						 nr_bases * sizeof(unsigned long));
	assert(percpu_bases != nullptr);
	percpu_bases[nr_bases - 1] = base;
}

void Init()
{
	size_t percpu_size = (unsigned long) &__percpu_end - (unsigned long) &__percpu_start;
	printf("percpu: .percpu size: %lu\n", percpu_size);

	void *buffer = zalloc(percpu_size);
	assert(buffer != nullptr);

	wrmsr(GS_BASE_MSR, (unsigned long) buffer);

	write_per_cpu(__cpu_base, (unsigned long) buffer);

	assert(get_per_cpu(__cpu_base) == (unsigned long) buffer);

	AddPercpu((unsigned long) buffer);
	percpu_init = true;
}

unsigned long InitForCpu(unsigned int cpu)
{
	size_t percpu_size = (unsigned long) &__percpu_end - (unsigned long) &__percpu_start;

	void *buffer = zalloc(percpu_size);
	assert(buffer != nullptr);

	AddPercpu((unsigned long) buffer);

	other_cpu_write(__cpu_base, (unsigned long) buffer, cpu);

	return (unsigned long) buffer;
}

bool percpu_initialized()
{
	return percpu_init;
}

}