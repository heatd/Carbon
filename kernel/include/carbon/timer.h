/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_TIMER_H
#define _CARBON_TIMER_H

#include <carbon/clocksource.h>

namespace Timer
{

class TimerEvent
{
private:
	using Callback = void (*)(void *context);
	ClockSource::ClockTicks ticks_in_the_future;
	Callback callback;
	void *context;
	bool can_run_in_irq;
public:
	TimerEvent(ClockSource::ClockTicks ticks_in_the_future,
		   Callback cb, void *context, bool can_run_in_irq) :
	ticks_in_the_future(ticks_in_the_future), callback(cb), context(context),
		can_run_in_irq(can_run_in_irq)
	{
	}

	~TimerEvent(){}

	inline bool DecrementTicks()
	{
		/* If we have to trigger in that moment,
		 * return true as ticks_in_the_future == 0
		*/
		if(--ticks_in_the_future == 0)
		{
			return true;
		}

		return false;
	}

	inline Callback GetCallback() const
	{
		return callback;
	}

	inline void *GetContext()
	{
		return context;
	}

	inline bool CanRunInIrq() const
	{
		return can_run_in_irq;
	}
};

bool AddTimerEvent(TimerEvent& event);
void HandlePendingTimerEvents();

};

#endif