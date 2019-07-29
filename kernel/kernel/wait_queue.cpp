/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/wait_queue.h>

void WaitQueue::WakeUpUnlocked()
{
	if(!head)
	{
		return;
	}
		
	auto thread = head;
	Scheduler::DequeueThread(this, thread);

	Scheduler::UnblockThread(thread);
}

void WaitQueue::WakeUp()
{
	AcquireLock();
	WakeUpUnlocked();
	ReleaseLock();
}

void WaitQueue::WakeUpAll()
{
	AcquireLock();

	while(head) WakeUpUnlocked();

	ReleaseLock();
}

void WaitQueue::Wait()
{
	Scheduler::EnqueueThread(this, get_current_thread());
	Scheduler::SetCurrentState(THREAD_BLOCKED);

	ReleaseLock();

	Scheduler::Yield();

	AcquireLock();
}

void WaitQueue::ReleaseLock()
{
	if(allow_irqs)
		spin_unlock(&lock);
	else
		spin_unlock_irqrestore(&lock);
}

void WaitQueue::AcquireLock()
{
	if(allow_irqs)
		spin_lock(&lock);
	else
		spin_lock_irqsave(&lock);
}