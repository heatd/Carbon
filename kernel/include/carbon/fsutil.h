/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_FSUTIL_H
#define _CARBON_FSUTIL_H

#include <stddef.h>

#include <carbon/inode.h>
#include <carbon/filesystem.h>

/* Note: Needs to virtually inheric from inode in order to:
 * 1) Override inode::read and inode::write and, most importantly
 * 2) enable derived classes to inherit from both inode and generic_non_backed_ino
 * this is known as the diamond problem
*/

/* Note2: generic_non_backed_ino isn't virtual anymore because it can't
 * virtually inherit inode and be the only base class for the derived class.
 * Multiple inheritance is weird.
*/
/* generic_non_backed_ino - implements methods for generic non backed
 * filesystems, like ramfs.
*/

class generic_fs_without_ino : public filesystem
{
private:
	ino_t ino;
public:
	generic_fs_without_ino(const char *name, dentry *root)
	: filesystem(name, root), ino(3) {}
	ino_t allocate_next_ino();
};

class generic_non_backed_ino : public inode
{
public:
	generic_non_backed_ino() : inode() {}
	ssize_t write(const void *buffer, size_t size, size_t off) override;
	ssize_t read(void *buffer, size_t size, size_t off) override;
	inode *create(const char *name, mode_t mode) override;
};

#endif