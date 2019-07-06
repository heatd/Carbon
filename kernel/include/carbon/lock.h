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
	unsigned long old_flags;
};

void spin_lock(struct spinlock *lock);
void spin_lock_irqsave(struct spinlock *lock);
void spin_unlock(struct spinlock *lock);
void spin_unlock_irqrestore(struct spinlock *lock);

class Spinlock
{
private:
	struct spinlock lock;
public:
	Spinlock(){};
	~Spinlock();
	void Lock();
	void LockIrqsave();
	void Unlock();
	void UnlockIrqrestore();
	bool IsLocked();
};

template <typename LockType, bool irq_save = false>
class ScopedLock
{
private:
	bool IsLocked;
	LockType *internal_lock;
public:
	void Lock()
	{
		if(irq_save)
			internal_lock->LockIrqsave();
		else
			internal_lock->Lock();
		IsLocked = true;
	}

	void Unlock()
	{
		if(irq_save)
			internal_lock->UnlockIrqrestore();
		else
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

using ScopedSpinlock = ScopedLock<Spinlock>;
using ScopedSpinlockIrqsave = ScopedLock<Spinlock, true>;

#endif
