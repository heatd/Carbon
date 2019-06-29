/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_CPU_H
#define _CARBON_CPU_H

#ifdef __x86_64__

#include <carbon/x86/apic.h>

#endif

struct Cpu
{
	unsigned long sched_quantum;
	struct Cpu *self;
#ifdef __x86_64__
	x86::Apic::Lapic *lapic;
	unsigned long apic_ticks;
#endif 	
};

Cpu *SetupPerCpuStruct();

inline Cpu *GetPerCpu()
{
	Cpu *c;
#ifdef __x86_64__
	__asm__ __volatile__("movq %%gs:0x8, %0":"=r"(c));
#else
	#error "Not implemented"
#endif
	return c;
}

#endif