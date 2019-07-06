/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/lock.h>
#include <carbon/scheduler.h>

struct mutex
{
	struct spinlock llock;
	struct thread *head;
	struct thread *tail;
	unsigned long counter;
	struct thread *owner;
};

void mutex_lock(struct mutex *mtx);
void mutex_unlock(struct mutex *mtx);

class Mutex
{
private:
	struct mutex lock;
public:
	constexpr Mutex() : lock {} {}
	~Mutex() {}
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
