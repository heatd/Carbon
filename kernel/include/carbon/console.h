/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_CONSOLE_H
#define _CARBON_CONSOLE_H

#include <stddef.h>

#include <sys/types.h>

struct console
{
	char *name;
	ssize_t (*write)(const void *buffer, size_t len, struct console *c);
	ssize_t (*read)(void *buffer, size_t len, struct console *c);
	void *priv;
	struct console *next;
};

void console_add(struct console *c);

/* This is not a bug! Only console_write is extern "C" */
#ifdef __cplusplus
extern "C"
#endif
ssize_t console_write(void *buffer, size_t len);


void console_reset(struct console *new_console);

#endif
