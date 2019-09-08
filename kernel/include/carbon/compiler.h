/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_COMPILER_H
#define _CARBON_COMPILER_H

#define align(x) __attribute__((aligned(x)))
#define __align_cache align(16)
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#define prefetch(...) __builtin_prefetch(__VA_ARGS__)
#define ASSUME_ALIGNED(x,y) __builtin_assume_aligned(x,y)
#define ilog2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)

#ifdef __x86_64__

#define USES_FANCY_START	_Pragma("GCC push_options") \
_Pragma("GCC target(\"sse2\", \"3dnow\", \"xsave\")")

#define USES_FANCY_END _Pragma("GCC pop_options")

#endif

#endif