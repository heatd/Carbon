/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <stdlib.h>

void *memccpy(void *restrict s1, const void *restrict s2, int c, size_t n)
{
	unsigned char *restrict dstptr = (unsigned char *restrict) s1;
	const unsigned char *restrict srcptr = (const unsigned char *restrict) s2;
	for(size_t i = 0; i < n; i++)
	{
		*dstptr++ = *srcptr++;
		if(*srcptr-1 == (unsigned char) c)
		{
			return (void*) dstptr;
		}
	}
	return NULL;
}