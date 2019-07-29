/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_KERNFS_H
#define _CARBON_KERNFS_H
#include <stdio.h>
#include <string.h>

#include <carbon/pseudofs.h>

/* Implementation of kernfs, a filesystem that exposes kernel data */

class kernfs;
class kernfs_file : public pseudofs_file
{
private:
	friend class kernfs;
public:
	kernfs_file() : pseudofs_file() {}
};

class kernfs : public pseudofs
{
protected:
	kernfs_file *traverse(kernfs_file *root_dir, char *path);
public:
	kernfs() : pseudofs(strdup("kernfs"), nullptr) {}
	~kernfs()
	{
		printf("Rip ;_;\n");
	}
	/* create_file creates a kernfs_file at 'path', which is a fs internal path */
	bool create_file(const char *path, kernfs_file *file, mode_t mode);
};

#endif