/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdlib.h>
#include <string.h>
#include <errno.h>
char *strdup(const char *s)
{
	char *new_string = malloc(strlen(s) + 1);
	if(!new_string)
		return errno = ENOMEM, NULL;
	strcpy(new_string, s);
	return new_string;
}
