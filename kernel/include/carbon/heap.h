/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_HEAP_H
#define _CARBON_HEAP_H

#include <stdint.h>
#include <stddef.h>

struct heap
{
	void *starting_address;
	void *brk;
	size_t size;
};

#ifdef __cplusplus
extern "C"
#endif
struct heap *heap_get();

#endif