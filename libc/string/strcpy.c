/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>

#include <carbon/compiler.h>

/* Copy the NULL-terminated string src into dest, and return dest. */
char *strcpy(char * restrict dest, const char * restrict src)
{
	char *ret = dest;
	while(*src != '\0')
		*dest++ = *src++;
	*dest = '\0';
	return ret;
}

char *strncpy(char * restrict dest, const char * restrict src, size_t n)
{
	char *ret = dest;
	size_t i = 0;
	for(i = 0; i < n && *src; i++)
		*dest++ = *src++;
	for(; i < n; i++)
		*dest++ = '\0';
	return ret;
}

size_t strlcpy(char * restrict dst, const char * restrict src, size_t n)
{
	const char *og_source = src;
	size_t to_copy = n;

	/* Algorithm taken from FreeBSD, https://github.com/freebsd/freebsd/blob/master/sys/libkern/strlcpy.c */
	if(likely(to_copy != 0))
	{
		while(--to_copy)
		{
			if((*dst++ = *src++) == 0)
				break;
		}
	}

	if(unlikely(to_copy == 0))
	{
		/* strlcpy was truncated */
		if(n != 0)
			*dst = '\0';
		while(*src++) {}
	}

	return src - og_source - 1;
}