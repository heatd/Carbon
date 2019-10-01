/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_HANDLE_H
#define _CARBON_HANDLE_H

#include <carbon/refcount.h>
#include <carbon/public/handle.h>

class process;
 
class handle
{
private:
	refcountable *kernel_object;
	unsigned long object_type;
	process *owner;
public:
	static constexpr unsigned long file_object_type = 0x10000;
	static constexpr unsigned long process_object_type = 0x10001;
	/* Note: handle::handle() refers the kernel object on its own */
	handle(refcountable *ko, unsigned long type, process *owner);
	~handle();
	handle(const handle&) = delete;
	handle(handle&& h)
	{
		kernel_object = h.kernel_object;
		object_type = h.object_type;
		owner = h.owner;

		h.kernel_object = nullptr;
		h.owner = nullptr;
		h.object_type = 0;
	}

	handle& operator=(const handle& h) = delete;
	handle& operator=(handle&& h)
	{
		kernel_object = h.kernel_object;
		object_type = h.object_type;
		owner = h.owner;

		h.kernel_object = nullptr;
		h.owner = nullptr;
		h.object_type = 0;

		return *this;
	}

	refcountable *get_object()
	{
		return kernel_object;
	}

	/* Note: handle::set_object() refers the kernel object on its own */
	void set_object(refcountable *ko);
	unsigned long get_object_type() const
	{
		return object_type;
	}

	process *get_owner() const
	{
		return owner;
	}

	void set_owner(process *p)
	{
		owner = p;
	}

	void unref();
};

#endif