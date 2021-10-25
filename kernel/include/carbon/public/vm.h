/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PUBLIC_VM_H
#define _CARBON_PUBLIC_VM_H

#include <stddef.h>

#define MAP_FLAG_FILE	(1 << 0)
#define MAP_FLAG_FIXED	(1 << 1)

#define MAP_PROT_READ	(1 << 0)
#define MAP_PROT_WRITE	(1 << 1)
#define MAP_PROT_EXEC	(1 << 2)

/* The calling convention forces to pack some arguments
 * together in a struct :/
*/
struct __cbn_mmap_packed_args
{
	size_t length;
	size_t off;
	long flags;
};

#endif