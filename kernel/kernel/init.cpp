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
#include <carbon/process.h>
#include <carbon/vterm.h>

void initrd_init(struct module *mod);

int kernel_init(struct boot_info *info)
{
	vterm_init_sysobj();

	page_cache::init();

	ramfs::mount_root();

	info->modules->start += PHYS_BASE;
	initrd_init(info->modules);

	shared_ptr<process_namespace> first_namespace = make_shared<process_namespace>(0);
	assert(first_namespace != nullptr);

	cbn_status_t st;

	int argc = 1;
	char *args[] = {"/init", NULL};
	char *env[] = {"INIT=1", NULL};
	auto p = process::kernel_spawn_process_helper("init", "/sbin/init",
				PROCESS_SPAWN_FLAG_CREATE_THREAD | PROCESS_SPAWN_FLAG_START,
				first_namespace, argc, args, 1, env, st);
	if(!p)
		printf("Failed with status %i\n", st);
	
	return 0;
}