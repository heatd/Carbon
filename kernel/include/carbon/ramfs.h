/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_RAMFS_H
#define _CARBON_RAMFS_H

#include <stdio.h>

#include <carbon/pseudofs.h>
#include <carbon/fsutil.h>

class ramfs_file : public generic_non_backed_ino
{
public:
	ramfs_file() : generic_non_backed_ino() {}
	~ramfs_file() {}
};

class ramfs : public generic_fs_without_ino
{
public:
	ramfs(const char *name) : generic_fs_without_ino(name, nullptr) {}
	~ramfs(){}

	static void mount_root();
};

#endif