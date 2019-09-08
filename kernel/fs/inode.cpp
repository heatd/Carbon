/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <errno.h>
#include <string.h>

#include <carbon/inode.h>
#include <carbon/memory.h>
#include <carbon/page.h>
#include <carbon/vmobject.h>

ssize_t inode::read(void *buffer, size_t size, size_t off)
{
	return errno = ENOSYS, -1;
}

ssize_t inode::write(const void *buffer, size_t size, size_t off)
{
	return errno = ENOSYS, -1;
}

inode *inode::open(const char *name)
{
	return errno = ENOSYS, nullptr;
}

inode *inode::create(const char *name, mode_t mode)
{
	return errno = ENOSYS, nullptr;
}

bool inode::create_vmobject_if_needed()
{
	if(i_pages)
		return true;

	if(!(i_pages = new vm_object_phys(true, i_size, nullptr)))
		return false;

	return true;
}

page_cache_block *inode::add_page(struct page *page, size_t size, size_t offset)
{
	if(!create_vmobject_if_needed())
		return nullptr;
	page_cache_block *block = new page_cache_block(page, size, offset, this);
	if(!block)
		return nullptr;

	page->misc_data.cache_block = block;
	if(i_pages->add_page(offset, page) < 0)
	{
		delete block;
		return nullptr;
	}

	if(!page_cache::append_page_cache_block(block))
	{
		i_pages->remove_page(offset);
		delete block;
		return nullptr;
	}

	return block;
}

page_cache_block *inode::do_caching(size_t off, long flags)
{
	page_cache_block *block = nullptr;
	/* Allocate a cache buffer */
	struct page *p = alloc_pages(1, 0);
	if(!p)
		return nullptr;
	
	void *virt = phys_to_virt(p->paddr);

	/* The size may be less than PAGE_SIZE, because we may reach EOF
	 * before reading a whole page */

	ssize_t size = read(virt, PAGE_SIZE, off);

	if(size <= 0 && !(flags & FILE_CACHING_WRITING))
	{
		free_page(p);
		return nullptr;
	}

	/* Add the cache block */
	block = add_page(p, (size_t) size, off);
	/* Now the block might be added, return. We don't need to check for
	 * null
	*/
	return block;
}

page_cache_block *inode::get_page_internal(size_t offset, long flags)
{
	size_t aligned_off = (offset / PAGE_SIZE) * PAGE_SIZE;
	if(!create_vmobject_if_needed())
		return nullptr;

	auto b = i_pages->get(aligned_off);

	if(b)
	{
		i_pages->lock.Unlock();
		return b->misc_data.cache_block;
	}

	/* Try to add it to the cache if it didn't exist before. */
	auto new_block = do_caching(aligned_off, flags);

	return new_block;
}

ssize_t inode::write_page_cache(const void *buffer, size_t len, size_t offset)
{
	const char *buf = reinterpret_cast<const char *>(buffer);
	size_t wrote = 0;
	do
	{
		page_cache_lock.Lock();

		auto cache = get_page_internal(offset, FILE_CACHING_WRITING);

		if(cache == nullptr)
		{
			page_cache_lock.Unlock();
			if(wrote)
			{
				return wrote;
			}
			else
				return -errno;
		}

		off_t cache_off = offset % PAGE_SIZE;
		off_t rest = PAGE_SIZE - cache_off;

		size_t amount = len - wrote < (size_t) rest ?
			len - wrote : (size_t) rest;


		cache->write((const void *) buf, amount, cache_off);
	
		if(cache->get_size() < cache_off + amount)
		{
			cache->set_size(cache_off + amount);
		}
	
		cache->set_dirty();

		offset += amount;
		wrote += amount;
		buf += amount;
		if(offset > i_size)
			i_size = offset;

		page_cache_lock.Unlock();
	} while(wrote != len);

	return (ssize_t) wrote;
}

ssize_t inode::read_page_cache(void *buffer, size_t len, size_t offset)
{
	size_t read = 0;

	while(read != len)
	{
		page_cache_lock.Lock();
		page_cache_block *cache = get_page_internal(offset, 0);

		if(!cache)
		{
			size_t ret = -errno;
			if(read)
			{
				ret = read;
			}
			page_cache_lock.Unlock();
			return ret;
		}

		off_t cache_off = offset % PAGE_SIZE;
		off_t rest = PAGE_SIZE - cache_off;

		assert(rest > 0);
	
		size_t amount = len - read < (size_t) rest ?
			len - read : (size_t) rest;

		if(offset + amount > i_size)
		{
			amount = i_size - offset;
			cache->read((void *) ((char *) buffer + read), amount, cache_off);
			page_cache_lock.Unlock();
			return read + amount;
		}
		else
			cache->read((void *) ((char *) buffer + read), amount, cache_off);
		offset += amount;
		read += amount;

		page_cache_lock.Unlock();
	}

	return (ssize_t) read;
}
