/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_FS_VFS_H
#define _CARBON_FS_VFS_H

#include <carbon/smart.h>
#include <carbon/inode.h>

namespace fs
{

enum open_flags
{
	none = 0,
	no_follow_symlinks = (1 << 0)
};

inode* open(const char *name, open_flags flags, inode* ino);
inode* create(const char *name, mode_t mode, inode* ino);
ssize_t write(const void *buffer, size_t size, size_t off, inode *ino);
ssize_t read(void *buffer, size_t size, size_t off, inode *ino);
inode *mkdir(const char *path, mode_t mode, inode *ino);

};

#endif