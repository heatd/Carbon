/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <assert.h>

#include <carbon/handle.h>
#include <carbon/handle_table.h>

handle::handle(refcountable *ko, unsigned long __object_type, process *__owner) :
	kernel_object(ko), object_type(__object_type), owner(__owner) 
{
	kernel_object->ref();
}

handle::~handle()
{
	unref();
}

void handle::set_object(refcountable *ko)
{
	assert(kernel_object == nullptr);
	kernel_object = ko;
	kernel_object->ref();
}

void handle::unref()
{
	kernel_object->unref();
	kernel_object = nullptr;
}

handle_table::~handle_table()
{
	for(auto it = handles.begin(); it != handles.end(); it++)
	{
		auto handle = *it;
		delete handle;
	}
}

cbn_handle_t handle_table::allocate_handle(handle *handle)
{
	scoped_rwlock<scoped_rwlock_write> guard{&handle_table_lock};

	auto handle_index = allocate_handle_index();

	if(handle_index == CBN_INVALID_HANDLE)
		return CBN_INVALID_HANDLE;
	
	if(handle_index >= handles.size())
	{
		if(!handles.push_back(handle))
		{
			handle_bitmap.FreeBit(handle_index);
			return CBN_INVALID_HANDLE;
		}
	}
	else
		handles[handle_index] = handle;

	nr_open_handles++;

	return handle_index;
}


cbn_handle_t handle_table::allocate_handle_index()
{
	unsigned long nr;
	if(nr_open_handles == handle_bitmap.GetSize())
	{
		auto new_size = handle_bitmap.GetSize() + HANDLE_BITMAP_EXPANSION_INCREMENT;
		if(!handle_bitmap.ReallocBitmap(new_size))
			return CBN_INVALID_HANDLE;
	}

	while(!handle_bitmap.FindFreeBit(&nr))
	{
		auto new_size = handle_bitmap.GetSize() + HANDLE_BITMAP_EXPANSION_INCREMENT;
		if(!handle_bitmap.ReallocBitmap(new_size))
			return CBN_INVALID_HANDLE;
	}

	if(nr > CBN_HANDLE_MAX)
	{
		handle_bitmap.FreeBit(nr);
		return CBN_INVALID_HANDLE;
	}

	return (cbn_handle_t) nr;
}

cbn_status_t handle_table::close_handle(cbn_handle_t handle)
{
	scoped_rwlock<scoped_rwlock_write> guard{&handle_table_lock};

	if(handle > handles.size())
		return CBN_STATUS_INVALID_HANDLE;
	if(!handles[handle])
		return CBN_STATUS_INVALID_HANDLE;
	
	auto h = handles[handle];
	handles[handle] = nullptr;
	handle_bitmap.FreeBit(handle);
	delete h;

	return CBN_STATUS_OK;
}

handle *handle_table::get_handle(cbn_handle_t id)
{
	scoped_rwlock<scoped_rwlock_read> guard{&handle_table_lock};

	if(id > handles.size())
		return nullptr;
	if(!handles[id])
		return nullptr;
	return handles[id];
}