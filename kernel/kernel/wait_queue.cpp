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
	spin_lock_irqsave(&lock);
	WakeUpUnlocked();
	spin_unlock_irqrestore(&lock);
}

void WaitQueue::WakeUpAll()
{
	spin_lock_irqsave(&lock);

	while(head) WakeUpUnlocked();

	spin_unlock_irqrestore(&lock);
}

void WaitQueue::Wait()
{
	Scheduler::EnqueueThread(this, get_current_thread());
	Scheduler::SetCurrentState(THREAD_BLOCKED);

	spin_unlock_irqrestore(&lock);

	Scheduler::Yield();

	spin_lock_irqsave(&lock);
}

void WaitQueue::ReleaseLock()
{
	spin_unlock_irqrestore(&lock);
}