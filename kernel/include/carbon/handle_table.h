/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_HANDLE_TABLE_H
#define _CARBON_HANDLE_TABLE_H

#include <carbon/handle.h>
#include <carbon/vector.h>
#include <carbon/bitmap.h>
#include <carbon/rwlock.h>
#include <carbon/smart.h>
#include <carbon/status.h>

#define HANDLE_BITMAP_EXPANSION_INCREMENT	1024

class handle_table
{
private:
	/* Choosing between handle and handle* is hard because it's an issue of 
	 * cache locality and having to keep the thing locked forever or having kinda
	 * crap locality but not needing to keep the thing locked for a long time.
	 * I went with the latter. */
	vector<shared_ptr<handle> > handles;
	Bitmap<0> handle_bitmap;
	rw_lock handle_table_lock;
	unsigned long nr_open_handles;
	cbn_handle_t allocate_handle_index();
public:
	handle_table() : handles{}, handle_bitmap{}, handle_table_lock{} {}
	~handle_table();

	cbn_handle_t allocate_handle(handle *handle);
	cbn_status_t close_handle(cbn_handle_t handle);
	shared_ptr<handle> get_handle(cbn_handle_t id);

	/* note: emplace_handle returns the old handle if it exists*/
	bool emplace_handle(handle *hndl, cbn_handle_t dst, shared_ptr<handle> &old, bool overwrite);
};

#endif