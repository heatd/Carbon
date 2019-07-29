/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PSEUDOFS_H
#define _CARBON_PSEUDOFS_H

#include <carbon/list.h>
#include <carbon/filesystem.h>

class pseudofs_file : public inode
{
protected:
	const char *name;
	/* If dir */
	/* TODO: Switch fs locks to rw locks */
	Spinlock children_lock;
	LinkedList<pseudofs_file *> children;
public:
	pseudofs_file() : inode() {}
	pseudofs_file *find_children(const char *name);
	bool add_child(pseudofs_file *file);
	inode *open(const char *name);
};

class pseudofs : public filesystem
{
protected:
	pseudofs_file *root_file;
public:
	pseudofs(const char *name, dentry *root) : filesystem(name, root) {}
	bool create_root_dentry()
	{
		root_file = new pseudofs_file();
		if(!root_file)
			return false;
		dentry *r = new dentry(strdup(""), root_file);
		if(!r)
			return false;
		root_dentry = r;

		return true;
	}
};

#endif