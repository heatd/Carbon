/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_FRAMEBUFFER_H
#define _CARBON_FRAMEBUFFER_H

#include <carbon/bootprotocol.h>

struct framebuffer
{
	const char *name;
	unsigned long framebuffer_phys;
	void *framebuffer;
	unsigned long framebuffer_size;
	unsigned long height;
	unsigned long width;
	unsigned long bpp;
	unsigned long pitch;
	struct color_info color;
};

struct framebuffer *get_primary_framebuffer(void);
void set_framebuffer(struct framebuffer *fb);

#endif
