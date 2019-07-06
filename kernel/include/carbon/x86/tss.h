/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_X86_TSS_H
#define _CARBON_X86_TSS_H

#include <stdint.h>

namespace Tss
{

void InitPercpu(uint64_t *gdt);
void SetKernelStack(unsigned long stack);

};

#endif