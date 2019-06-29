#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <carbon/console.h>

static char print_buffer[200000] = {0};

/* Define errno somewhere */
int errno;

int printf(const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	int result = vsnprintf(print_buffer, sizeof(print_buffer), fmt, list);
	va_end(list);

	console_write(print_buffer, strlen(print_buffer));
	memset(print_buffer, 0, sizeof(print_buffer));
	return result;
}

int puts(const char *s)
{
	return printf("%s\n", s);
}
