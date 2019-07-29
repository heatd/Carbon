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

#ifdef __cplusplus
extern "C" {
#endif

void spin_lock(struct spinlock *lock);
void spin_lock_irqsave(struct spinlock *lock);
void spin_unlock(struct spinlock *lock);
void spin_unlock_irqrestore(struct spinlock *lock);

#ifdef __cplusplus
}

class Spinlock
{
private:
	struct spinlock lock;
public:
	constexpr Spinlock() : lock {0, 0} {};
	~Spinlock();
	void Lock();
	void LockIrqsave();
	void Unlock();
	void UnlockIrqrestore();
	bool IsLocked();
};

template <typename LockType, bool irq_save = false>
class scoped_lock
{
private:
	bool IsLocked;
	LockType *internal_lock;
public:
	void lock()
	{
		if(irq_save)
			internal_lock->LockIrqsave();
		else
			internal_lock->Lock();
		IsLocked = true;
	}

	void unlock()
	{
		if(irq_save)
			internal_lock->UnlockIrqrestore();
		else
			internal_lock->Unlock();
		IsLocked = false;
	}

	scoped_lock(LockType *lock) : internal_lock(lock)
	{
		this->lock();
	}

	~scoped_lock()
	{
		if(IsLocked)
			unlock();
	}
};

using scoped_spinlock = scoped_lock<Spinlock>;
using scoped_spinlockIrqsave = scoped_lock<Spinlock, true>;

#endif

#endif
