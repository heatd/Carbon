#ifndef _CARBON_PANIC_H
#define _CARBON_PANIC_H

#ifdef __cplusplus
extern "C" 

[[noreturn]]
#endif

void panic(const char *message);

#ifdef __cplusplus

#include <stdio.h>

template <typename T>
[[noreturn]]
void panic_bounds_check(T* arr, bool is_vec, unsigned long bad_index)
{
	const char *type = is_vec ? "vector" : "array";
	printf("%s::operator[] detected a bad access with index %lu\n",
		type, bad_index);
	printf("%s address: %p\n", type, arr);
	panic("array bounds check");
}
#endif

#endif