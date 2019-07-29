/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <carbon/fsutil.h>

ssize_t generic_non_backed_ino::write(const void *buffer, size_t size, size_t off)
{
	return size;
}

ssize_t generic_non_backed_ino::read(void *buffer, size_t size, size_t off)
{
	return 0;
}

inode *generic_non_backed_ino::create(const char *name, mode_t mode)
{
	/* Note: We don't need to create dentries because the dentry code
	 * does it for us
	*/
	generic_non_backed_ino *ino = new generic_non_backed_ino();
	if(!ino)
		return errno = ENOMEM, nullptr;
	
	/* TODO: Set dev, ctime, etc */
	ino->i_mode = mode;
	ino->i_fs = this->i_fs;
	generic_fs_without_ino *i = (generic_fs_without_ino *) ino->i_fs;
	ino->i_ino = i->allocate_next_ino();
	ino->ref();

	if(!ino->i_fs->add_inode_to_table(ino))
	{
		delete ino;
		return errno = ENOMEM, nullptr;
	}

	return ino;
}
