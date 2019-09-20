/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/


#ifndef _CARBON_SYSTEM_OBJECT_H
#define _CARBON_SYSTEM_OBJECT_H

#include <carbon/list.h>
#include <carbon/lock.h>
#include <carbon/refcount.h>
#include <carbon/status.h>

class sysobj
{
private:
	char *name;
	LinkedList<sysobj*> children;
	Spinlock children_lock;
	refcountable *object;
	bool valid_object;
	unsigned long object_type;
public:
	sysobj(char *n, refcountable *object, bool valid_object, unsigned long type)
		: name(n), object(object), valid_object(valid_object), object_type(type) {}

	~sysobj();

	refcountable *get_obj() {return object;}
	unsigned long get_objtype() {return object_type;}
	bool append_child(sysobj *to_append);
	sysobj *open_child_object(char *name);
	char *get_name() {return name;}
};

namespace sysobjs
{
	void set_root(sysobj *obj);
	cbn_status_t open_object(char *full_name, sysobj **result);
};

#endif