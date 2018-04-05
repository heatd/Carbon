/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
 #include <string.h>

int strcmp(const char *s, const char *t)
{
	int i;
	for (i = 0; s[i] == t[i]; i++)
		if (s[i] == '\0')
			return 0;
	return s[i] - t[i];
}

int strncmp(const char *s, const char *t, size_t n)
{
	int i;
	size_t c = 0;
	for (i = 0; s[i] == t[i] && c < n; i++, c++)
		if (s[i] == '\0')
			return 0;
	return s[i] - t[i];
}
