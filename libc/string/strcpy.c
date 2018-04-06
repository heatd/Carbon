/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
 #include <string.h>
/* Copy the NULL-terminated string src into dest, and return dest. */
char *strcpy(char *dest, const char *src)
{
	char *ret = dest;
	while(*src != '\0')
		*dest++ = *src++;
	*dest = '\0';
	return ret;
}
char *strncpy(char *dest, const char *src, size_t n)
{
	char *ret = dest;
	while(n)
	{
		*dest++ = *src++;
		n--;
	}
	*dest = '\0';
	return ret;
}
