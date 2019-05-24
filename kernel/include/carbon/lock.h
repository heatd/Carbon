/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_LOCK_H
#define _CARBON_LOCK_H

#include <stdint.h>
#include <assert.h>

struct spinlock
{
	unsigned long lock;
};

void spin_lock(struct spinlock *lock);
void spin_unlock(struct spinlock *lock);

class Spinlock
{
private:
	struct spinlock lock;
public:
	Spinlock() : lock({0}){};
	~Spinlock();
	void Lock();
	void Unlock();
	bool IsLocked();
};

template <typename LockType>
class ScopedLock
{
private:
	bool IsLocked;
	LockType *internal_lock;
public:
	void Lock()
	{
		internal_lock->Lock();
		IsLocked = true;
	}

	void Unlock()
	{
		internal_lock->Unlock();
		IsLocked = false;
	}

	ScopedLock(LockType *lock) : internal_lock(lock)
	{
		Lock();
	}

	~ScopedLock()
	{
		if(IsLocked)
			Unlock();
	}
};

typedef ScopedLock<Spinlock> ScopedSpinlock;

#endif
