/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_SYSCALL_UTILS_H
#define _CARBON_SYSCALL_UTILS_H

#include <carbon/status.h>
#include <carbon/process.h>

extern "C" cbn_status_t copy_to_user(void *udst, const void *ksrc, size_t size);
extern "C" cbn_status_t copy_from_user(void *kdst, const void *usrc, size_t size);
extern "C" size_t strlen_user(const char *ustr);

class kernel_string
{
private:
	char *raw_string;
	size_t len;
public:
	kernel_string() : raw_string{nullptr}, len{0} {}
	~kernel_string()
	{
		if(raw_string)
			free((void *) raw_string);
	}

	cbn_status_t from_user_string(const char *ustring);

	char *get_string()
	{
		return raw_string;
	}
	
	size_t get_len()
	{
		return len;
	}
};

#endif