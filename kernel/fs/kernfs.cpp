/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>

#include <carbon/kernfs.h>

kernfs_file *kernfs::traverse(kernfs_file *root_dir, char *path)
{
	char *saveptr;
	kernfs_file *dir = root_dir;

	char *p = strtok_r(path, PATH_SEPARATOR, &saveptr);

	while(p != nullptr)
	{
		dir = (kernfs_file *) dir->find_children(p);
		if(!dir)
		{
			errno = ENOENT;
			return nullptr;
		}
	
		p = strtok_r(nullptr, PATH_SEPARATOR, &saveptr);
	}

	return nullptr;
}

bool kernfs::create_file(const char *path, kernfs_file *file, mode_t mode)
{
	char *to_traverse = strdup(path);
	if(!to_traverse)
		return false;

	/* get the path to open in order to create the file */
	to_traverse = dirname(to_traverse);

	/* TODO: This down-casting is not safe, fix. */
	kernfs_file *dir = (kernfs_file *) root_file;

	/* if strlen(to_traverse) is 1, then to_traverse can only
	 * be '/' or '.', both of which = root_file in this case
	*/
	if(strlen(to_traverse) != 1)
		dir = traverse(dir, to_traverse);
	
	if(!dir)
		return false;
	

	if(!dir->add_child(file))
	{
		return false;
	}

	file->i_mode = mode;
 
	return true;
}