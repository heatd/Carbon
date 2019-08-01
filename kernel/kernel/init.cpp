/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <carbon/inode.h>
#include <carbon/kernfs.h>
#include <carbon/smart.h>
#include <carbon/pagecache.h>
#include <carbon/ramfs.h>
#include <carbon/fs/root.h>
#include <carbon/fs/vfs.h>
#include <carbon/rwlock.h>
#include <carbon/handle_table.h>
#include <carbon/utf8.h>

void initrd_init(struct module *mod);

int kernel_init(struct boot_info *info)
{
	page_cache::init();

	ramfs::mount_root();

	info->modules->start += PHYS_BASE;
	initrd_init(info->modules);

	return 0;
}