/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <carbon/timer.h>
#include <carbon/list.h>
#include <carbon/lock.h>
#include <carbon/panic.h>

namespace Timer
{

static LinkedList<TimerEvent*> pending_list;
static Spinlock list_lock;

bool AddTimerEvent(TimerEvent& event)
{
	scoped_spinlockIrqsave guard {&list_lock};
	TimerEvent *ev = new TimerEvent(event);
	if(!ev)
		return false;
	bool st = pending_list.Add(ev);

	if(!st)
		delete ev;
	return st;
}

void HandleRunningEvent(TimerEvent *event, LinkedListIterator<TimerEvent *> it)
{
	pending_list.Remove(event, it);

	auto callback = event->GetCallback();
	auto context = event->GetContext();

	if(event->CanRunInIrq())
		callback(context);
	else
	{
		panic("do something");
	}
}

void HandlePendingTimerEvents()
{
	scoped_spinlockIrqsave guard {&list_lock};
	for(auto it = pending_list.begin();
	    it != pending_list.end(); )
	{
		auto event = *it;

		bool needs_to_run = event->DecrementTicks();

		/* Note: We increment the iterator here since HandleRunningEvent
		 * removes the event from the list. Therefore if we did it in the for loop,
		 * it would be corrupted since the node it would point to wouldn't exist
		*/
		if(needs_to_run)
		{
			HandleRunningEvent(event, it++);
		}
		else
			it++;
	}
}

}