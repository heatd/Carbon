/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _KERNEL_FNV_H
#define _KERNEL_FNV_H

#include <stdint.h>
#include <stddef.h>
typedef uint64_t fnv_hash_t;

#define FNV_PRIME 		1099511628211
#define FNV_OFFSET_BASIS 	0xcbf29ce484222325

static inline fnv_hash_t __fnv_hash(uint8_t *data, size_t size)
{
	fnv_hash_t hash = FNV_OFFSET_BASIS;
	while(size--)
	{
		hash *= FNV_PRIME;
		hash ^= *data++;
	}
	return hash;
}

#define fnv_hash(data, size)	__fnv_hash((uint8_t *) data, size)

#endif
