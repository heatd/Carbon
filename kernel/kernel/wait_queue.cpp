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
	scheduler::dequeue_thread(this, thread);

	scheduler::unblock_thread(thread);
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
	scheduler::enqueue_thread(this, get_current_thread());
	scheduler::set_current_state(THREAD_BLOCKED);

	ReleaseLock();

	scheduler::yield();

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