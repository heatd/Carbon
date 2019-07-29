/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <carbon/dentry.h>
#include <carbon/lock.h>
#include <carbon/inode.h>
#include <carbon/filesystem.h>

dentry* dentry::open(const char *name)
{
	auto inode = underlying_inode->open(name);
	if(!inode)
		return nullptr;

	const char *n = strdup(name);
	if(!n)
	{
		inode->close();
		return nullptr;
	}

	dentry* d = new dentry(n, inode);
	if(!d)
	{
		free((void *) n);
		inode->close();
		return nullptr;
	}

	if(!children.Add(d))
	{
		delete d;
		inode->close();
		return nullptr;
	}

	if(S_ISDIR(inode->i_mode))
		inode->i_dentry = d;

	return d;
}

dentry *dentry::lookup_unlocked(const char *name)
{
	for(auto& dentry : children)
	{
		if(!strcmp(dentry->name, name))
			return dentry;
	}

	/* If the inode wasn't cached yet, get it */
	return open(name);
}

dentry* dentry::lookup(const char *name)
{
	scoped_rwlock<scoped_rwlock_read> guard{&lock};

	return lookup_unlocked(name);
}

void dentry::tear_down()
{
	scoped_rwlock<scoped_rwlock_write> guard{&lock};

	for(auto it = children.begin(); it != children.end();)
	{
		auto dentry = *it;
		auto to_delete = it++;
		children.Remove(dentry, to_delete);
		delete dentry;
	}
}

bool dentry::add_dentry_unlocked(dentry *dent)
{
	if(!children.Add(dent))
		return false;
	dent->parent = this;

	return true;
}

bool dentry::add_dentry(dentry *dent)
{
	scoped_rwlock<scoped_rwlock_write> guard{&lock};
	return add_dentry_unlocked(dent);
}