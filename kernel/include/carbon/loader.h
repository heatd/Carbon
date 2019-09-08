/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_LOADER_H
#define _CARBON_LOADER_H

#include <carbon/status.h>

#include <carbon/fs/vfs.h>
#include <carbon/array.h>

class process;

namespace program_loader
{

using binary_entry_point_t = void *;

/* Expected to be used by calling code before and after calling load()
 * in order to set up the thread */
struct binary_info
{
	process *proc;
	binary_entry_point_t entry;
	unsigned long args[4];
	/* 4 args should be enough.
	*  NOTE: These arguments aren't the ones in argv,
	*  but argc, argv, envp, etc themselves.
	*/
};

constexpr size_t sample_size = 256;

class binary_loader
{
public:
	virtual bool match(const array<uint8_t, sample_size>& sample) const = 0;
	virtual cbn_status_t load(fs::auto_inode& file, binary_info& out) const = 0;
};

cbn_status_t load(const char *path, process *process, binary_info& out_info);
bool add_loader(binary_loader* loader);

};

#endif