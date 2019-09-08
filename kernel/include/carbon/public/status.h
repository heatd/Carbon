/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PUBLIC_STATUS_H
#define _CARBON_PUBLIC_STATUS_H


enum cbn_status_t : int
{
	CBN_STATUS_OK = 0,
	CBN_STATUS_INVALID_HANDLE = -1,
	CBN_STATUS_OUT_OF_MEMORY = -2,
	CBN_STATUS_RSRC_LIMIT_HIT = -3,
	CBN_STATUS_INVALID_ARGUMENT = -4,
	CBN_STATUS_SEGFAULT = -5,
	CBN_STATUS_DOES_NOT_EXIST = -6,
	CBN_STATUS_INTERNAL_ERROR = -7
};


#endif