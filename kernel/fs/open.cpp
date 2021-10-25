/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#include <carbon/fs/vfs.h>
#include <carbon/filesystem.h>

/*******************************************************************************************
 * File: open.cpp - Implements vfs-level open, create and close
*******************************************************************************************/

namespace fs
{

dentry *open_direntry(const char *path, open_flags flags, dentry *base)
{
	char *saveptr;
	shared_ptr<char> to_traverse(strdup(path));

	if(!to_traverse)
		return errno = ENOMEM, nullptr;

	char *tok = strtok_r(to_traverse.get_data(), PATH_SEPARATOR, &saveptr);
	auto curr = base;

	while(tok)
	{
		curr = curr->lookup(tok);
		if(!curr)
		{
			return errno = ENOENT, nullptr;
		}

		tok = strtok_r(nullptr, PATH_SEPARATOR, &saveptr);
	}

	return curr;
}

inode* open(const char *path, open_flags flags, inode* ino)
{
	if(!S_ISDIR(ino->i_mode))
		return errno = ENOTDIR, nullptr;
	auto direntry = ino->i_dentry;

	auto d = open_direntry(path, flags, direntry);
	if(!d)
		return nullptr;
	return d->get_inode();
}

inode* create(const char *path, mode_t mode, inode* ino)
{
	if(!S_ISDIR(ino->i_mode))
		return errno = ENOTDIR, nullptr;

	char *path_dup = strdup(path);

	if(!path_dup)
		return errno = ENOMEM, nullptr;
	
	/* get the path to open in order to create the file */
	auto to_traverse = dirname(path_dup);
	auto direntry = ino->i_dentry;

	/* if strlen(to_traverse) is 1, then to_traverse can only
	 * be '/' or '.', both of which = root_file in this case
	*/

	if(strlen(to_traverse) != 1)
		direntry = open_direntry(to_traverse, open_flags::none, direntry);

	if(!direntry)
	{
		free((void *) path_dup);
		return nullptr;
	}

	auto dir_inode = direntry->get_inode();

	/* now, get the to-be-created file's name */
	strcpy(path_dup, path);

	auto name = strdup(basename(path_dup));
	if(!name)
	{
		free((void *) path_dup);
		return nullptr;
	}

	dentry *d = new dentry(name, nullptr);
	if(!d)
	{
		free((void *) path_dup);
		delete d;
		return nullptr;
	}

	d->lock_dir();

	if(!direntry->add_dentry_unlocked(d))
	{
		delete d;
		free((void *) path_dup);
		return nullptr;
	}

	auto new_inode = dir_inode->create(name, mode);
	if(!new_inode)
	{
		d->remove_from_parent_unlocked();
		delete d;
		free((void *) path_dup);
		return nullptr;
	}

	d->set_inode(new_inode);
	
	if(S_ISDIR(new_inode->i_mode))
		new_inode->i_dentry = d;

	d->unlock_dir();

	free((void *) path_dup);

	return new_inode;
}

inode *mkdir(const char *path, mode_t mode, inode *ino)
{
	return create(path, mode | S_IFDIR, ino);
}

void close(inode *ino)
{
	ino->close();
}

}