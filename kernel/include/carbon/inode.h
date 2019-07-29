/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_INODE_H
#define _CARBON_INODE_H

#include <sys/stat.h>
#include <stddef.h>
#include <sys/types.h>

#include <carbon/hashtable.h>
#include <carbon/fnv.h>
#include <carbon/lock.h>
#include <carbon/pagecache.h>

#include <carbon/refcount.h>
inline fnv_hash_t page_cache_block_hash(page_cache_block*& p)
{
	size_t off = p->get_offset();
	return fnv_hash(&off, sizeof(off));
}

#define INODE_NR_PAGE_CACHE_HASHTABLE_ENTRIES	1024
class dentry;
class filesystem;

class inode : public refcountable
{
private:
	page_cache_block *add_page(struct page *p, size_t size, size_t off);
public:
	dev_t i_dev;
	ino_t i_ino;

	mode_t i_mode;
	uid_t i_uid;
	gid_t i_gid;
	dev_t i_rdev;
	size_t i_size;
	filesystem *i_fs;
	struct timespec i_atim;
	struct timespec i_mtim;
	struct timespec i_ctim;
	/* i_dentry is valid only if this is a directory, because hard-links to directories are impossible */
	dentry *i_dentry;

	hashtable<page_cache_block*, INODE_NR_PAGE_CACHE_HASHTABLE_ENTRIES,
		  fnv_hash_t, page_cache_block_hash> i_pages{};
	Spinlock page_cache_lock{};
	inode() : refcountable{0}, i_dev{0}, i_ino{0}, i_mode{0}, i_uid{0}, i_gid{0},
		  i_rdev{0}, i_size{0}, i_fs{nullptr}, i_atim{}, i_mtim{}, i_ctim{},
		  i_dentry(nullptr) {}

	inode(const inode&) = delete;
	inode(inode&&) = delete;
	virtual ~inode()
	{}

	#define FILE_CACHING_WRITING		(1 << 0)
	page_cache_block *get_page(size_t off, long flags);
	page_cache_block *get_page_internal(size_t off, long flags);
	page_cache_block *do_caching(size_t off, long flags);
	ssize_t read_page_cache(void *buf, size_t len, size_t off);
	ssize_t write_page_cache(const void *buf, size_t len, size_t off);

	virtual ssize_t read(void *buffer, size_t size, size_t off);
	virtual ssize_t write(const void *buffer, size_t size, size_t off);
	virtual inode *open(const char *name);
	virtual inode *create(const char *name, mode_t mode);

	void close()
	{
		unref();
	}
};

#endif