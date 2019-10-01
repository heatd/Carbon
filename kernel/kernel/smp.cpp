/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>

#include <carbon/smp.h>
#include <carbon/bitmap.h>
#include <carbon/memory.h>
#include <carbon/percpu.h>


extern unsigned char _start_smp;
extern unsigned char _end_smp;

namespace Smp
{

static Bitmap<0, false> bt;
unsigned int nr_cpus = 0;
unsigned int online_cpus = 0;
constexpr unsigned long smp_trampoline_phys = 0x0;

PER_CPU_VAR(unsigned int cpu_nr) = 0;

void SetNumberOfCpus(unsigned int nr)
{
	bt.SetSize(nr);
	assert(bt.AllocateBitmap() == true);
	nr_cpus = nr;
}

void SetOnline(unsigned int cpu)
{
	bt.SetBit(cpu);
	online_cpus++;
}

void BootCpus()
{
	printf("smpboot: booting cpus\n");
	memcpy((void*) (PHYS_BASE + (uintptr_t) smp_trampoline_phys), &_start_smp,
		(uintptr_t) &_end_smp - (uintptr_t) &_start_smp);
	
	for(unsigned int i = 0; i < nr_cpus; i++)
	{
		if(!bt.IsSet(i))
		{
			Boot(i);
		}
	}

	printf("smpboot: done booting cpus, %u online\n", online_cpus);
}

unsigned int GetOnlineCpus()
{
	return online_cpus;
}

}