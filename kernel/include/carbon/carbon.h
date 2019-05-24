/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_H
#define _CARBON_H

#include <carbon/bare_handle.h>

#define container_of(ptr, type, member)	\
((type *) ((char*) ptr - offsetof(type, member)))

#define align(x) __attribute__((aligned(x)))
#define __align_cache align(16)
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#define prefetch(...) __builtin_prefetch(__VA_ARGS__)
#define ASSUME_ALIGNED(x,y) __builtin_assume_aligned(x,y)
#define ARCH_SPECIFIC extern
#define UNUSED_PARAMETER(x) (void)x
#define UNUSED(x) UNUSED_PARAMETER(x)
#define __init __attribute__((constructor))
#define weak_alias(name, aliasname) _weak_alias (name, aliasname)
#define _weak_alias(name, aliasname) \
extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

#endif
