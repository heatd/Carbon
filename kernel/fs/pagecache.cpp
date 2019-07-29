/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <carbon/memory.h>
#include <carbon/pagecache.h>
#include <carbon/inode.h>
#include <carbon/scheduler.h>
#include <carbon/wait_queue.h>
#include <carbon/lock.h>
#include <carbon/list.h>

void page_cache_block::set_dirty()
{
	dirty = 1;
	page_cache::wake_flush_thread();
}

int page_cache_block::flush()
{
	const void *buffer = phys_to_virt(page->paddr);
	size_t st = ino->write(buffer, size, offset);

	if(st != size)
		return -1;

	dirty = 0;
	return 0;
}

int page_cache_block::write(const void *buffer, size_t size, size_t off)
{
	char *buf = (char *) phys_to_virt(page->paddr);

	memcpy(buf + off, buffer, size);

	return 0;
}

ssize_t page_cache_block::read(void *buf, size_t len, size_t off)
{
	size_t to_read = len;
	if(to_read + off > size)
	{
		to_read = size - off;
	}

	char *p = (char *) phys_to_virt(page->paddr);
	assert(to_read <= PAGE_SIZE);
	memcpy(buf, p + off, to_read);

	return to_read;
}

page_cache_block::~page_cache_block()
{
	if(dirty)
		flush();
	free_page(page);
}

namespace page_cache
{

thread *flush_thread = nullptr;
LinkedList<page_cache_block *> page_cache_list;
WaitQueue page_cache_wait {true};

void wake_flush_thread()
{
	page_cache_wait.WakeUp();
}

bool append_page_cache_block(page_cache_block *block)
{
	page_cache_wait.AcquireLock();
	bool st = page_cache_list.Add(block);
	page_cache_wait.ReleaseLock();
	return st;
}

void flush_pages()
{
	for(auto page_cache_block : page_cache_list)
	{
		if(page_cache_block->is_dirty())
		{
			int st = page_cache_block->flush();

			if(st < 0)
			{
				printf("page_cache: flush() for block %p,"
				       "inode %p failed\n", page_cache_block,
				       page_cache_block->get_inode());
				printf("errno: %u\n", errno);
			}
		}
	}
}

void flush_main(void *context)
{
	/* 
	 * The pagecache daemon thread needs to loop forever and complete a run
	 * when it is woken up, or in other words, when a block has been
	 * written to and is marked dirty 
	*/

	/* The comment above was copied from Onyx but it still holds true.
	 * Damn, past me was smart as hell.
	*/

	page_cache_wait.AcquireLock();

	/* In between the creation of this thread and right now, something
	 * might have gotten dirty, so just do a quick flushing run
	*/

	flush_pages();
	while(true)
	{
		/* Great, we're in the loop and we're ready to be sleepy and
		 * wake up when needed
		*/
		page_cache_wait.Wait();
		flush_pages();
	}
}

void init()
{
	flush_thread = Scheduler::CreateThread(page_cache::flush_main,
					       nullptr,
					       Scheduler::CREATE_THREAD_KERNEL);
	assert(flush_thread != nullptr);

	Scheduler::StartThread(flush_thread);
}

}

namespace fs
{

size_t write(const void *buffer, size_t size, size_t off, inode *ino)
{
	if(!S_ISREG(ino->i_mode))
		return ino->write(buffer, size, off);
	return ino->write_page_cache(buffer, size, off);
}

size_t read(void *buffer, size_t size, size_t off, inode *ino)
{
	if(!S_ISREG(ino->i_mode))
		return ino->read(buffer, size, off);
	return ino->read_page_cache(buffer, size, off);
}

}