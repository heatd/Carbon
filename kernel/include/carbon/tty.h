/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_TTY_H
#define _CARBON_TTY_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <sys/types.h>


struct tty
{
	const char *name;
	int (*write)(const char *buf, size_t n, unsigned int flags);
	ssize_t (*read)(char *buf, size_t sizeofread, unsigned int flags);
	struct tty *next;
	void *priv;
};

int tty_add(struct tty *tty);

#endif
