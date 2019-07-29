/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_DENTRY_H
#define _CARBON_DENTRY_H

#include <carbon/mutex.h>
#include <carbon/list.h>
#include <carbon/inode.h>
#include <carbon/smart.h>
#include <carbon/rwlock.h>

#include <stdio.h>

class dentry
{
private:
	const char *name;
	inode* underlying_inode;
	dentry* parent;
	LinkedList <dentry* > children;
	/* TODO: Use a rwlock */
	rw_lock lock;
	dentry* lookup_unlocked(const char *name);
	dentry* open(const char *name);
public:
	dentry(const char *name, inode* inode) : name(name),
		underlying_inode(inode), children{}, lock{} {}

	dentry* lookup(const char *name);
	void tear_down();

	void remove_from_parent_unlocked()
	{
		parent->children.Remove(this);
	}

	void remove_from_parent()
	{
		scoped_rwlock<scoped_rwlock_write> guard {&parent->lock};
		remove_from_parent_unlocked();
	}

	~dentry()
	{
		delete name;
		tear_down();
		underlying_inode->close();
		underlying_inode = nullptr;

	}

	inode* get_inode() const
	{
		return underlying_inode;
	}

	void lock_dir()
	{
		lock.lock_write();
	}

	void unlock_dir()
	{
		lock.unlock_write();
	}

	bool add_dentry_unlocked(dentry *dent);
	bool add_dentry(dentry *dent);

	void set_inode(inode *ino)
	{
		underlying_inode = ino;
	}
};

#endif