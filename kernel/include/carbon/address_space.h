/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_ADDRESS_SPACE_H
#define _CARBON_ADDRESS_SPACE_H

#include <carbon/lock.h>
struct rb_tree;

struct address_space
{
	unsigned long start;
	unsigned long end;
	struct rb_tree *area_tree;
	Spinlock lock;
	void *arch_priv;
};

#endif