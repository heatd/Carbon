/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_RWLOCK_H
#define _CARBON_RWLOCK_H

#include <assert.h>

#include <carbon/scheduler.h>
#include <carbon/lock.h>
#include <stdio.h>

class rw_lock
{
public:
	thread *head, *tail;
	Spinlock llock;
	unsigned long counter;
	constexpr static unsigned long counter_locked_write = (unsigned long) -1;
	constexpr static unsigned long max_readers = counter_locked_write - 1;

	constexpr rw_lock() : head(nullptr), tail(nullptr), llock{}, counter(0) {}
	void lock_read();
	bool trylock_read();
	void lock_write();
	bool trylock_write();
	void unlock_read();
	void unlock_write();
};

enum scoped_rwlock_type
{
	scoped_rwlock_read,
	scoped_rwlock_write
};

template <scoped_rwlock_type type>
class scoped_rwlock
{
private:
	rw_lock *l;
	bool is_locked;
public:
	void lock_read()
	{
		static_assert(type != scoped_rwlock_write, "can't use lock_read in a read scoped_rwlock");
		l->lock_read();
		is_locked = true;
	}

	void lock_write()
	{
		static_assert(type != scoped_rwlock_read, "can't use lock_write in a write scoped_rwlock");
		l->lock_write();
		is_locked = true;
	}

	void unlock_read()
	{
		static_assert(type != scoped_rwlock_write, "can't use unlock_read in a read scoped_rwlock");
		l->unlock_read();
		is_locked = false;
	}

	void unlock_write()
	{
		static_assert(type != scoped_rwlock_read, "can't use lock_write in a write scoped_rwlock");
		l->unlock_write();
		is_locked = false;
	}

	void lock()
	{
		if constexpr(type == scoped_rwlock_read)
			lock_read();
		else
			lock_write();
	}

	void unlock()
	{
		if constexpr(type == scoped_rwlock_read)
			unlock_read();
		else
			lock_write();
	}

	constexpr scoped_rwlock(rw_lock *__l) : l(__l)
	{
		lock();
	}

	~scoped_rwlock()
	{
		if(is_locked)
		{
			unlock();
		}
	}
};

#endif