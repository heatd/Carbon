/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_STATUS_H
#define _CARBON_STATUS_H

#include <carbon/public/status.h>

#include <errno.h>

inline cbn_status_t errno_to_cbn_status_t(int errno_val)
{
	switch(errno_val)
	{
		case ENOMEM:
			return CBN_STATUS_OUT_OF_MEMORY;
		case ENOENT:
			return CBN_STATUS_DOES_NOT_EXIST;
		case EINVAL:
			return CBN_STATUS_INVALID_ARGUMENT;
		case ESRCH:
			return CBN_STATUS_RSRC_LIMIT_HIT;
		case EFAULT:
			return CBN_STATUS_SEGFAULT;
		default:
			return CBN_STATUS_INTERNAL_ERROR;	/* not yet translated */
	}
}

#endif