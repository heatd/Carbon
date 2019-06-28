/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdint.h>
#include <cpuid.h>
#include <stdio.h>

#include <carbon/x86/cpu.h>

namespace x86
{

namespace Cpu
{

struct cpu cpu = {};

void Identify(void)
{
	uint32_t eax = 0;
	uint32_t ebx = 0;
	uint32_t ecx = 0;
	uint32_t edx = 0;
	if(!__get_cpuid(CPUID_FEATURES, &eax, &ebx, &ecx, &edx))
	{
		printf("x86cpu: CPUID_FEATURES not supported!\n");
	}
	cpu.caps[0] = edx | ((uint64_t) ecx << 32);

	eax = CPUID_FEATURES_EXT;
	ecx = 0;
	if(!__get_cpuid(CPUID_FEATURES_EXT, &eax, &ebx, &ecx, &edx))
	{
		printf("x86cpu: CPUID_FEATURES_EXT not supported!\n");
	}
	cpu.caps[1] = ebx | ((uint64_t) ecx << 32);
	cpu.caps[2] = edx;
	eax = CPUID_EXTENDED_PROC_INFO;
	if(!__get_cpuid(CPUID_EXTENDED_PROC_INFO, &eax, &ebx, &ecx, &edx))
	{
		printf("x86cpu: CPUID_EXTENDED_PROC_INFO not supported!\n");
	}
	cpu.caps[2] |= ((uint64_t) edx) << 32;
	cpu.caps[3] = ecx;

	if(!__get_cpuid(CPUID_ADVANCED_PM, &eax, &ebx, &ecx, &edx))
	{
		printf("x86cpu: CPUID_ADVANCED_PM not supported!\n");
	}

	cpu.invariant_tsc = (bool) (edx & (1 << 8));

}

const int bits_per_long = sizeof(unsigned long) * 8;

__attribute__((hot))
bool HasCap(int cap)
{
	/* Get the index in native word sizes(DWORDS in 32-bit systems and QWORDS in 64-bit ones) */
	int q_index = cap / bits_per_long;
	int bit_index = cap % bits_per_long;
	return cpu.caps[q_index] & (1UL << bit_index);
}

};

};
