/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_FILESYSTEM_H
#define _CARBON_FILESYSTEM_H

#include <assert.h>
#include <carbon/dentry.h>
#include <carbon/dentry.h>
#include <carbon/smart.h>
#include <carbon/lock.h>

inline fnv_hash_t inode_hash(inode*& p)
{
	auto ino = p->i_ino;
	return fnv_hash(&ino, sizeof(ino));
}

class filesystem
{
protected:
	const char *name;
	unsigned long ref;
	dentry* root_dentry;
	cul::hashtable<inode *, 1024, fnv_hash_t, inode_hash> inode_table;
	Spinlock ino_table_lock{};
public:
	filesystem(const char *name, dentry *root) : name(name), ref(0),
	root_dentry(root), inode_table{} {}
	~filesystem()
	{
		assert(ref == 0);
		delete name;
	}

	bool create_root_dentry(inode* root_inode);

	dentry* get_root() const
	{
		return root_dentry;
	}

	bool add_inode_to_table(inode *ino);
	inode *get_inode(ino_t ino);
	int unmount();
};

#define PATH_SEPARATOR "/"

#endif