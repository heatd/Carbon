/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <carbon/lock.h>
#include <carbon/scheduler.h>
#ifdef __x86_64__
#include <carbon/x86/eflags.h>
#endif

void spin_lock(struct spinlock *lock)
{
	scheduler::disable_preemption();
	while(__sync_lock_test_and_set(&lock->lock, 1))
	{
		while(lock->lock == 1)
			__asm__ __volatile__("pause");
	}
}

void spin_lock_irqsave(struct spinlock *lock)
{
	lock->old_flags = irq_save_and_disable();
	spin_lock(lock);
}

void spin_unlock(struct spinlock *lock)
{
	__sync_lock_release(&lock->lock);

	scheduler::enable_preemption();
}

void spin_unlock_irqrestore(struct spinlock *lock)
{
	unsigned long f = lock->old_flags;
	spin_unlock(lock);
	irq_restore(f);
}

Spinlock::~Spinlock()
{
	assert(lock.lock != 1);
}

void Spinlock::Lock()
{
	spin_lock(&lock);
}

void Spinlock::LockIrqsave()
{
	spin_lock_irqsave(&lock);
}

void Spinlock::Unlock()
{
	spin_unlock(&lock);
}

void Spinlock::UnlockIrqrestore()
{
	spin_unlock_irqrestore(&lock);
}

bool Spinlock::IsLocked()
{
	return lock.lock;
}