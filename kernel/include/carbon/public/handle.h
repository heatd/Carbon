/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PUBLIC_HANDLE_H
#define _CARBON_PUBLIC_HANDLE_H

#include <stdint.h>

#include <carbon/status.h>

typedef uint32_t cbn_handle_t;

#define CBN_INVALID_HANDLE		(uint32_t) -1
#define CBN_HANDLE_MAX			UINT32_MAX

#define CBN_OPEN_ACCESS_READ		(1 << 0)
#define CBN_OPEN_ACCESS_WRITE		(1 << 1)

cbn_status_t cbn_open_sys_handle(const char *upath, unsigned long permitions);

#endif