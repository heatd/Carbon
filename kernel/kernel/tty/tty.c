/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <carbon/tty.h>

struct tty *ttys = NULL;

int tty_add(struct tty *tty)
{
	struct tty **pp = &ttys;

	while(*pp)
	{
		pp = &(*pp)->next;
	}

	*pp = tty;

	return 0;
}
