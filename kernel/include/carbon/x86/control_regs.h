/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_X86_CONTROL_REGS_H
#define _CARBON_X86_CONTROL_REGS_H

#include <carbon/compiler.h>

#define CR4_OSXSAVE		(1 << 18)

namespace x86
{

inline unsigned long ReadCr0()
{
	unsigned long val;
	__asm__ __volatile__("mov %%cr0, %0":"=r"(val));
	return val;
}

inline unsigned long ReadCr2()
{
	unsigned long val;
	__asm__ __volatile__("mov %%cr2, %0":"=r"(val));
	return val;
}

inline unsigned long ReadCr3()
{
	unsigned long val;
	__asm__ __volatile__("mov %%cr3, %0":"=r"(val));
	return val;
}

inline unsigned long ReadCr4()
{
	unsigned long val;
	__asm__ __volatile__("mov %%cr4, %0":"=r"(val));
	return val;
}

inline void WriteCr0(unsigned long val)
{
	__asm__ __volatile__("mov %0, %%cr0"::"r"(val));
}

inline void WriteCr2(unsigned long val)
{
	__asm__ __volatile__("mov %0, %%cr2"::"r"(val));
}

inline void WriteCr3(unsigned long val)
{
	__asm__ __volatile__("mov %0, %%cr3"::"r"(val));
}

inline void WriteCr4(unsigned long val)
{
	__asm__ __volatile__("mov %0, %%cr4"::"r"(val));
}

}
#endif