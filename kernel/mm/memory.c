/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stddef.h>

#include <carbon/memory.h>

extern unsigned char kernel_end;

static unsigned char *kernel_break = &kernel_end;

void *ksbrk(size_t size)
{
	/* Align the size to a multiple of 16 */
	size = (size + 16) & -16;
	unsigned char *ret = kernel_break;
	kernel_break += size;
	return ret;
}
