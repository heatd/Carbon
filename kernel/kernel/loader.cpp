/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/loader.h>
#include <carbon/filesystem.h>
#include <carbon/fs/vfs.h>
#include <carbon/fs/root.h>
#include <carbon/vector.h>

namespace program_loader
{

vector<program_loader::binary_loader*> loader_list;
rw_lock loader_list_lock;

bool add_loader(binary_loader* loader)
{
	scoped_rwlock<scoped_rwlock_write> guard{&loader_list_lock};

	if(!loader_list.push_back(loader))
		return false;

	return true;
}

const binary_loader* find_loader(array<uint8_t, sample_size>& sample)
{
	scoped_rwlock<scoped_rwlock_read> guard{&loader_list_lock};

	for(auto it = loader_list.cbegin(); it != loader_list.cend(); ++it)
	{
		auto& l = *it;
		if(l->match(sample) == true)
			return l;
	}
	
	return nullptr;
}

cbn_status_t load(const char *path, process *process, binary_info& out_info)
{
	auto root_dentry = get_root();
	auto root_inode = root_dentry->get_inode();

	auto file = fs::open(path, fs::open_flags::none, root_inode);
	if(!file)
	{
		return errno_to_cbn_status_t(errno);
	}

	fs::auto_inode guard{file};
	array<uint8_t, sample_size> *buf = new array<uint8_t, sample_size>;
	if(!buf)
	{
		return CBN_STATUS_OUT_OF_MEMORY;
	}
	
	auto st = fs::read(buf->data, sample_size, 0, guard);
	if(st != (ssize_t) sample_size)
	{
		delete buf;
		return errno_to_cbn_status_t(errno);
	}

	auto loader = find_loader(*buf);
	if(!loader)
	{
		delete buf;
		return CBN_STATUS_INVALID_ARGUMENT;
	}

	delete buf;

	out_info.proc = process;
	auto ret_status = loader->load(guard, out_info);

	return ret_status;
}

};