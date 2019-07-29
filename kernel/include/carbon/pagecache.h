/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PAGECACHE_H
#define _CARBON_PAGECACHE_H

#include <carbon/page.h>
#include <carbon/smart.h>
#include <carbon/memory.h>

class inode;

class page_cache_block
{
	struct page *page;
	size_t size;
	size_t offset;
	volatile unsigned long dirty;
	inode* ino;
public:
	page_cache_block(struct page *page, size_t size, size_t offset,
			 inode* ino) : 
		page(page), size(size), offset(offset), dirty(0), ino(ino)
	{}

	~page_cache_block();

	/* set_dirty() - sets the page as dirty and allows it to get written to 
	 * the backing storage.
	*/
	void set_dirty();

	inode* get_inode() const
	{
		return ino;
	}

	bool is_dirty() const
	{
		return dirty;
	}

	size_t get_offset() const
	{
		return offset;
	}

	size_t get_size() const
	{
		return size;
	}

	void set_size(size_t _size)
	{
		size = _size;
	}

	const void *get_buf() const
	{
		return phys_to_virt(page->paddr);
	}

	/* flush - flushes the page */
	int flush();

	/* write - writes to the page
	 * NOTE: size and off are page cache block relative
	 * NOTE2: write does not set the page as dirty since the caller might
	 * want to group multiple write to a single page as a flush. */
	int write(const void *p, size_t size, size_t off);

	ssize_t read(void *buf, size_t size, size_t off);
};

namespace page_cache
{
	void init();
	bool append_page_cache_block(page_cache_block *block);
	void wake_flush_thread();
};

#endif