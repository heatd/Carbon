/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <string.h>

#include <carbon/filesystem.h>
#include <carbon/fsutil.h>

bool filesystem::create_root_dentry(inode* root_inode)
{
	char *root_name = strdup("");
	if(!root_name)
		return false;
	
	auto d = new dentry(root_name, root_inode);
	if(!d)
	{
		delete root_name;
		return false;
	}

	root_dentry = d;
	root_inode->i_dentry = root_dentry;

	return true;
}

static dentry* root_dentry;

dentry* get_root()
{
	return root_dentry;
}

void set_root(dentry* dent)
{
	root_dentry = dent;
}

ino_t generic_fs_without_ino::allocate_next_ino()
{
	return __sync_fetch_and_add(&ino, 1);
}

bool filesystem::add_inode_to_table(inode *ino)
{
	scoped_spinlock guard{&ino_table_lock};
	ino->ref();
	return inode_table.add_element(ino);
}

inode *filesystem::get_inode(ino_t ino)
{
	auto hash = fnv_hash(&ino, sizeof(ino));

	scoped_spinlock guard{&ino_table_lock};

	auto it = inode_table.get_hash_list_begin(hash);
	auto end = inode_table.get_hash_list_end(hash);

	for(; it != end; ++it)
	{
		auto in = *it;
		if(in->i_ino == ino)
		{
			in->ref();
			return in;
		}		
	}

	return nullptr;
}

int filesystem::unmount()
{
	scoped_spinlock guard{&ino_table_lock};

	delete root_dentry;
	root_dentry = nullptr;
	inode_table.empty();

	return 0;
}