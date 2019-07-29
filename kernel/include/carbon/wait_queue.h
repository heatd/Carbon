/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_WAIT_QUEUE_H
#define _CARBON_WAIT_QUEUE_H

#include <carbon/lock.h>
#include <carbon/scheduler.h>

struct WaitQueue
{
	struct spinlock lock;
	struct thread *head;
	struct thread *tail;
	const bool allow_irqs;

	constexpr WaitQueue(bool allow_irqs = true)
		: lock{}, head{nullptr}, tail{nullptr}, allow_irqs{allow_irqs} {}
	~WaitQueue() {}

	void WakeUpUnlocked();
	void WakeUp();
	void WakeUpAll();
	void Wait();
	void ReleaseLock();
	void AcquireLock();
};

#endif