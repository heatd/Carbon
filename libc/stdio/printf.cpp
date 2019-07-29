#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <carbon/console.h>
#include <carbon/lock.h>

static char print_buffer[200000] = {0};
static Spinlock print_lock;

extern "C"
int printf(const char *fmt, ...)
{
	scoped_spinlock guard {&print_lock};
	va_list list;
	va_start(list, fmt);
	int result = vsnprintf(print_buffer, sizeof(print_buffer), fmt, list);
	va_end(list);

	console_write(print_buffer, strlen(print_buffer));
	memset(print_buffer, 0, sizeof(print_buffer));
	return result;
}

extern "C"
int puts(const char *s)
{
	return printf("%s\n", s);
}
