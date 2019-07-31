/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PROCESS_H
#define _CARBON_PROCESS_H


#include <carbon/scheduler.h>
#include <carbon/list.h>
#include <carbon/handle_table.h>

class process
{
private:
	LinkedList<thread> thread_list;
	const char *name;	
public:
	process() : thread_list{}, name{} {}
	~process();

	int set_name(const char *string);
	const char *get_name() const;
};


#endif