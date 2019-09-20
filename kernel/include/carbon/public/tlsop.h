/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PUBLIC_TLSOP_H
#define _CARBON_PUBLIC_TLSOP_H

#include <carbon/public/status.h>

enum tls_op : unsigned long
{
	get_gs = 0,
	get_fs,
	set_gs,
	set_fs
};

#ifdef __cplusplus
extern "C" {
#endif

/* Works like linux's arch_prctl - addr is unsigned long on set, unsigned long * on get */
cbn_status_t cbn_tls_op(tls_op operation, unsigned long addr);

#ifdef __cplusplus
}
#endif

#endif