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
	CBN_STATUS_HANDLE_NOT_FOUND = -1,
	CBN_STATUS_INVALID_HANDLE = -2
};


#endif