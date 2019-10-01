/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <assert.h>

#include <carbon/process.h>
#include <carbon/handle.h>
#include <carbon/handle_table.h>
#include <carbon/syscall_utils.h>
#include <carbon/utility.hpp>

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
		handle = nullptr;
	}
}

cbn_handle_t handle_table::allocate_handle(handle *h)
{
	scoped_rwlock<scoped_rwlock_write> guard{&handle_table_lock};

	auto handle_index = allocate_handle_index();

	if(handle_index == CBN_INVALID_HANDLE)
		return CBN_INVALID_HANDLE;

	if(handle_index >= handles.size())
	{
		if(!handles.push_back(shared_ptr<handle>{h}))
		{
			handle_bitmap.FreeBit(handle_index);
			return CBN_INVALID_HANDLE;
		}
	}
	else
		handles[handle_index] = shared_ptr<handle>{h};

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
	nr_open_handles--;

	return CBN_STATUS_OK;
}

shared_ptr<handle> handle_table::get_handle(cbn_handle_t id)
{
	scoped_rwlock<scoped_rwlock_read> guard{&handle_table_lock};

	if(id >= handles.size())
		return nullptr;
	if(!handles[id])
		return nullptr;
	return handles[id];
}

bool handle_table::emplace_handle(handle *hndl, cbn_handle_t dst, shared_ptr<handle> &old, bool overwrite)
{
	scoped_rwlock<scoped_rwlock_write> guard{&handle_table_lock};
	if(dst > handles.size())
	{
		if(!handles.alloc_buf(dst * sizeof(shared_ptr<handle>)))
			return false;
	}

	old = handles[dst];
	if(old && !overwrite)
		return false;

	handles[dst] = hndl;

	return true;
}

process *get_process_from_handle(cbn_handle_t process_handle)
{
	auto current = get_current_process();
	auto &current_handle_table = current->get_handle_table();

	process *dest_process = nullptr;
	if(process_handle == CBN_INVALID_HANDLE)
	{
		dest_process = get_current_process();
	}
	else
	{
		auto handle = current_handle_table.get_handle(process_handle);
		if(!handle)
			return nullptr;
		if(handle->get_object_type() != handle::process_object_type)
			return nullptr;
		dest_process = static_cast<process *>(handle->get_object());
	}

	return dest_process;
}

cbn_status_t sys_cbn_duplicate_handle(cbn_handle_t process_handle, cbn_handle_t srchandle_id,
	cbn_handle_t dsthandle, cbn_handle_t *result, unsigned long flags)
{
	auto dest_process = get_process_from_handle(process_handle);
	if(!dest_process)
		return CBN_STATUS_INVALID_HANDLE;
	auto &dest_handle_table = dest_process->get_handle_table();

	auto srchandle = dest_handle_table.get_handle(srchandle_id);
	if(!srchandle)
		return CBN_STATUS_INVALID_ARGUMENT;

	/* Be careful, right now we only need to do this in order to copy */
	auto dup = new handle{srchandle->get_object(), srchandle->get_object_type(),
		srchandle->get_owner()};
	if(!dup)
		return CBN_STATUS_OUT_OF_MEMORY;
	
	cbn_handle_t dst = dsthandle;

	if(flags & CBN_DUPLICATE_HANDLE_ALLOC_HANDLE)
	{
		dst = dest_handle_table.allocate_handle(dup);
		if(dst == CBN_INVALID_HANDLE)
		{
			delete dup;
			return CBN_STATUS_OUT_OF_MEMORY;
		}

		*result = dst;
		return CBN_STATUS_OK;
	}
	
	/* old_handle gets deleted by RAII if it's valid */
	shared_ptr<handle> old_handle;
	bool success = dest_handle_table.emplace_handle(dup, dsthandle,
		old_handle, flags & CBN_DUPLICATE_HANDLE_OVERWRITE);

	if(success)
	{
		*result = dst;
		return CBN_STATUS_OK;
	}
	else
	{
		delete dup;
		/* TODO: Don't assume we ran out of memory */
		return CBN_STATUS_OUT_OF_MEMORY;
	}
}

cbn_status_t cbn_close_handle(cbn_handle_t process_handle, cbn_handle_t hndl)
{
	auto dest_process = get_process_from_handle(process_handle);
	if(!dest_process)
		return CBN_STATUS_INVALID_HANDLE;
	
	auto &dest_handle_table = dest_process->get_handle_table();

	return dest_handle_table.close_handle(hndl);
}

shared_ptr<handle> get_handle_from_handle_id(cbn_handle_t hid, unsigned long obj_type)
{
	auto current = get_current_process();
	auto &handle_table = current->get_handle_table();

	auto h = handle_table.get_handle(hid);
	if(!h)
		return nullptr;
	
	if(h->get_object_type() != obj_type)
		return nullptr;
	
	return h;
}