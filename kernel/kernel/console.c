/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stddef.h>

#include <carbon/console.h>
#include <carbon/lock.h>

struct spinlock list_lock;
struct console *console_list = NULL;

/* 
 * Console subsystem
 * 
 * 
 * Each struct console represent a destination, and the subsystem is designed
 * to support multiple destination ttys/whatever.
 * To write to the console, you use console_write(), which takes care of race
 * conditions, and writes to every single console.
 *  
 * void console_add(struct console *c)
 * 	Adds a console destination, holding the list_lock.
 * 
 * ssize_t console_write(void *buffer, size_t len)
 * 	Writes to every console destination, holding the list lock.
 * 
 * void console_reset(struct console *new_console)
 * 	While taking care of race conditions, resets the console subsystem by
 * setting console_list to new_console, therefore resetting the console list to
 * new_console.
 * 
*/


/* Add a console destination */
void console_add(struct console *c)
{
	spin_lock(&list_lock);
	struct console **pp = &console_list;

	while(*pp)
	{
		pp = &(*pp)->next;
	}
	
	*pp = c;

	spin_unlock(&list_lock);
}

ssize_t console_write(void *buffer, size_t len)
{
	spin_lock(&list_lock);

	ssize_t written = 0;
	for(struct console *c = console_list; c != NULL; c = c->next)
	{
		written += c->write(buffer, len, c);
	}

	spin_unlock(&list_lock);

	return written > (ssize_t) len ? (ssize_t) len : written; 
}

void console_reset(struct console *new_console)
{
	spin_lock(&list_lock);

	console_list = new_console;

	spin_unlock(&list_lock);
}
