#include <stdio.h>
#include <stdarg.h>

#include <carbon/tty.h>

static char print_buffer[200000] = {0};

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
