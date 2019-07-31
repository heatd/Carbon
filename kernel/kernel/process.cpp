/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <string.h>

#include <carbon/process.h>

int process::set_name(const char *name)
{
	const char *process_name = strdup(name);
	if(!process_name)
		return -1;
	this->name = process_name;

	return 0;
}

const char *process::get_name() const
{
	return name;
}

process::~process()
{
	
}