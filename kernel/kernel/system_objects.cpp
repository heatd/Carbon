/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdlib.h>
#include <string.h>

#include <carbon/handle.h>
#include <carbon/system_objects.h>
#include <carbon/syscall_utils.h>
#include <carbon/status.h>
#include <carbon/compiler.h>

sysobj::~sysobj()
{
	free((void *) name);
	if(object) object->unref();
}

bool sysobj::append_child(sysobj *obj)
{
	scoped_spinlock guard{&children_lock};

	return children.Add(obj);
}

sysobj *sysobj::open_child_object(char *name)
{
	scoped_spinlock guard{&children_lock};

	for(auto child : children)
	{
		if(!strcmp(name, child->get_name()))
			return child;
	}

	return nullptr;
}

namespace sysobjs
{

sysobj *obj_root = nullptr;

void set_root(sysobj *obj)
{
	obj_root = obj;
}

__init
void init()
{
	sysobj *root = new sysobj{"", nullptr, false, (unsigned long) -1};
	assert(root != nullptr);
	set_root(root);

	sysobj *carbon_subdir = new sysobj{"carbon", nullptr, false, (unsigned long) -1};
	assert(carbon_subdir != nullptr);
	assert(root->append_child(carbon_subdir) == true);
}

cbn_status_t open_object(char *full_name, sysobj **result)
{
	char *saveptr = nullptr;
	
	if(!obj_root)
		return CBN_STATUS_DOES_NOT_EXIST;

	char *duplicate = strdup(full_name);
	if(!duplicate)
		return CBN_STATUS_OUT_OF_MEMORY;
	char *token = nullptr;

	token = strtok_r(duplicate, ".", &saveptr);
	auto obj = obj_root;

	while(token != nullptr)
	{
		obj = obj->open_child_object(token);
		if(!obj)
		{
			free((void *) duplicate);
			return CBN_STATUS_DOES_NOT_EXIST;
		}

		token = strtok_r(nullptr, ".", &saveptr);
	}

	free(duplicate);
	*result = obj;

	return CBN_STATUS_OK;
}

}

cbn_status_t sys_cbn_open_sys_handle(const char *upath, unsigned long permitions)
{
	auto current = get_current_process();

	auto& handle_table = current->get_handle_table();

	kernel_string str;
	auto st = str.from_user_string(upath);

	if(st != CBN_STATUS_OK)
	{
		return st;
	}
	
	sysobj *result;
	st = sysobjs::open_object(str.get_string(), &result);

	if(st != CBN_STATUS_OK)
		return st;
	
	if(result->get_obj() == nullptr)
		return CBN_STATUS_INVALID_ARGUMENT;
	
	handle *h = new handle{result->get_obj(), result->get_objtype(), current};
	if(!h)
		return CBN_STATUS_OUT_OF_MEMORY;
	
	auto r = handle_table.allocate_handle(h);
	if(r == CBN_INVALID_HANDLE)
	{
		delete h;
		return CBN_STATUS_OUT_OF_MEMORY;
	}

	return (cbn_status_t) r;
}