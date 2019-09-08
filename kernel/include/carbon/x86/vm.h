/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_X86_VM_H
#define _CARBON_X86_VM_H

namespace vm
{

enum class limits : unsigned long
{
	user_min = 0x400000,
	user_max = 0x00007fffffffffff,
	user_stack_min = 0x0000500000000000,
	kernel_min = 0xffff800000000000,
	kernel_max = 0xffffffffffffffff
};

enum class defaults : unsigned long
{
	user_stack_size = 0x100000
};

void switch_address_space(void *pml);

};

#endif