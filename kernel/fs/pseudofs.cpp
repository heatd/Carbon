/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <errno.h>

#include <carbon/pseudofs.h>

pseudofs_file *pseudofs_file::find_children(const char *name)
{
	scoped_spinlock guard {&children_lock};

	for(auto child : children)
	{
		if(!strcmp(child->name, name))
			return child;
	}

	return nullptr;
}

bool pseudofs_file::add_child(pseudofs_file *file)
{
	scoped_spinlock guard {&children_lock};
	return children.Add(file);
}

inode *pseudofs_file::open(const char *name)
{
	auto f = find_children(name);
	if(!f)
		return errno = ENOENT, nullptr;
	return f;
}