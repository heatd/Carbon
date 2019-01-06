/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>

void __assert_fail(const char * assertion, const char * file,
	unsigned int line, const char * function)
{
	printf("Assertion %s failed in %s:%u, in function %s\n", assertion,
		file, line, function);
	__asm__ __volatile__("cli;hlt");
}

void panic(char *msg)
{
	printf("panic: %s\n", msg);

	__asm__ __volatile__("cli;hlt");
}
