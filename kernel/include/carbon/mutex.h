/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/lock.h>
#include <carbon/scheduler.h>

struct raw_mutex
{
	struct spinlock llock;
	struct thread *head;
	struct thread *tail;
	unsigned long counter;
	struct thread *owner;
};

void mutex_lock(struct raw_mutex *mtx);
void mutex_unlock(struct raw_mutex *mtx);

class mutex
{
private:
	struct raw_mutex lock;
public:
	constexpr mutex() : lock {} {}
	~mutex() {}
	inline void Lock()
	{
		mutex_lock(&lock);
	}

	inline void LockIrqsave()
	{
		Lock();
	}

	inline void Unlock()
	{
		mutex_unlock(&lock);
	}

	inline void UnlockIrqrestore()
	{
		Unlock();
	}

	inline bool IsLocked()
	{
		return lock.counter == 1;
	}
};
