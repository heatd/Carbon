/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_PROCESS_ARGS_H
#define _CARBON_PROCESS_ARGS_H

#include <stddef.h>

class process;

class process_args
{
private:
	int argc;
	char **kargv;
	int envc;
	char **kenvp;
	void *kbin_args;
	unsigned long kbinargsize;
	void *ustack_top;
	char *ustack_pointer;
	char *ustack_pointer_base;
	void (*kbin_dtor)(void *kbin_args){};
	process *proc;
public:
	explicit process_args(process *process, int argc, char **kargv, int envc, char **kenvp,
		unsigned long kbinargsize, void *kbin_args);
	void set_ustack_top(void *_ustack_top)
	{
		this->ustack_top = _ustack_top;
	}

	bool copy_strings_and_put_in_array(char *&string_space, char **kargs, char **uargs, int count);
	size_t string_array_len(int count, char **arg);

	bool copy_to_stack();
	bool copy_args();
	bool copy_env();
	bool copy_binargs();

	void set_kbin_args(void *kbin_args)
	{
		this->kbin_args = kbin_args;
	}

	void set_kbin_arg_size(unsigned long sz)
	{
		kbinargsize = sz;
	}

	void set_kbin_dtor(void (*dtor)(void *))
	{
		kbin_dtor = dtor;
	}

	void *get_ustack_pointer_base()
	{
		return ustack_pointer_base;
	}

	~process_args();
};

#endif