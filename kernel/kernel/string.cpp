/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/syscall_utils.h>

cbn_status_t kernel_string::from_user_string(const char *ustring)
{
	auto ustring_len = strlen_user(ustring);

	if(ustring_len == (size_t) CBN_STATUS_SEGFAULT)
		return CBN_STATUS_SEGFAULT;
	
	raw_string = (char *) malloc(ustring_len + 1);
	if(!raw_string)
		return CBN_STATUS_OUT_OF_MEMORY;

	if(copy_from_user((void *) raw_string, ustring, ustring_len) != CBN_STATUS_OK)
	{
		free((void *) raw_string);
		return CBN_STATUS_SEGFAULT;
	}

	raw_string[ustring_len] = '\0';

	len = ustring_len;

	return CBN_STATUS_OK;
}