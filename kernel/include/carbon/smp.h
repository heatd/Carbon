/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_SMP_H
#define _CARBON_SMP_H

#include <stddef.h>

#include <carbon/percpu.h>

namespace Smp
{

extern unsigned int cpu_nr;
void SetNumberOfCpus(unsigned int nr);
void SetOnline(unsigned int cpu);
void Boot(unsigned int cpu);
unsigned int GetOnlineCpus();

void BootCpus();

};

inline unsigned int get_cpu_nr()
{
	return get_per_cpu(Smp::cpu_nr);
}

#endif