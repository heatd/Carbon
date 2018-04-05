/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <carbon/lock.h>

void spin_lock(struct spinlock *lock)
{
	while(__sync_lock_test_and_set(&lock->lock, 1))
	{
		__asm__ __volatile__("pause");
	}
}

void spin_unlock(struct spinlock *lock)
{
	__sync_lock_release(&lock->lock);
}
