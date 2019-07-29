/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <errno.h>
#include <string.h>
#include <carbon/ramfs.h>

#include <carbon/fs/root.h>
#include <carbon/fs/vfs.h>

void ramfs::mount_root()
{
	auto fs = new ramfs(strdup("ramfs"));
	assert(fs);

	/* Ramfs checklist: Reading and writing stuff - partially done, implement at vfs level */
	/* dirent stuff - not done */
	inode* ino;
	
	{
		ramfs_file *f = new ramfs_file();
		f->i_mode = S_IFDIR | 0777;
		f->i_fs = fs;
		f->i_ino = 2;
		f->ref();
		ino = f;
	}

	assert(fs->create_root_dentry(ino) != false);

	set_root(fs->get_root());
}