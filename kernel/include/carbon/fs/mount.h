/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_FS_MOUNT_H
#define _CARBON_FS_MOUNT_H

#include <carbon/dentry.h>
#include <carbon/filesystem.h>

class vfs_mount
{
public:
	dentry *mountpoint;
	filesystem *mounted_fs;
	long flags;

	vfs_mount(dentry *dentry, filesystem *fs, long flags) :
		mountpoint(dentry), mounted_fs(fs), flags(flags) {}
	~vfs_mount() {}
};

#endif