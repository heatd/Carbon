/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_X86_GDT_H
#define _CARBON_X86_GDT_H

#include <stdint.h>

namespace Gdt
{

struct Gdtr
{
	uint16_t size;
	uint64_t ptr;
} __attribute__((packed));

void InitPercpu();

};

#endif