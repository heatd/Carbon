/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_SCHEDULER_H
#define _CARBON_SCHEDULER_H

#include <stddef.h>

#include <carbon/registers.h>
#include <carbon/clocksource.h>

namespace Scheduler
{

#define THREAD_RUNNABLE		0
#define THREAD_BLOCKED		1
#define THREAD_DEAD		2

#define NR_PRIO			40

#define SCHED_PRIO_VERY_LOW	0
#define SCHED_PRIO_LOW		10
#define SCHED_PRIO_NORMAL	20
#define SCHED_PRIO_HIGH		30
#define SCHED_PRIO_VERY_HIGH	39

struct thread
{
	size_t thread_id;
	unsigned long *kernel_stack;
	unsigned long *kernel_stack_top;
	unsigned long flags;
	unsigned long status;
	unsigned int priority;
	unsigned int cpu;
	int thread_errno;
	unsigned char *fpu_area;
	struct thread *prev, *next;
	struct thread *wait_prev, *wait_next;
#ifdef __x86_64__
	unsigned long fs;
	unsigned long gs;
#endif
};

#define THREAD_FLAG_KERNEL		(1 << 0)
#define THREAD_FLAG_NEEDS_RESCHED	(1 << 1)

using ThreadCallback = void (*)(void *context);

enum CreateThreadFlags : unsigned int
{
	CREATE_THREAD_KERNEL = THREAD_FLAG_KERNEL
};

struct thread *CreateThread(ThreadCallback callback, void *context,
			    CreateThreadFlags flags);
struct thread *CreateThread(struct registers *regs, CreateThreadFlags flags);
void StartThread(struct thread *thread, unsigned int cpu = -1);

bool ArchCreateThread(struct thread *thread, ThreadCallback callback,
		      void *context, CreateThreadFlags flags);
bool ArchCreateThreadLowLevel(struct thread *thread, struct registers *regs);


void ArchLoadThread(struct thread *thread);

void ArchSaveThread(struct thread *thread);

void OnTick();

void Initialize();

struct registers *Schedule(struct registers *regs);

void DisablePreemption();
void EnablePreemption();

void Yield();

void Sleep(ClockSource::ClockTicks ticks);

void SetupCpu(unsigned int cpu);

void UnblockThread(struct thread *thread);

void Block(struct thread *t);

extern struct thread *current_thread;

template <typename T>
void EnqueueThread(T *s, struct thread *thread)
{
	if(!s->head)
	{
		s->head = s->tail = thread;
		thread->wait_prev = thread->wait_next = NULL;
	}
	else
	{
		s->tail->wait_next = thread;
		thread->wait_prev = s->tail;
		s->tail = thread;
		thread->wait_next = NULL;
	}
}

template <typename T>
void DequeueThread(T *s, struct thread *thread)
{
	if(s->head == thread)
	{
		s->head = thread->wait_next;
		if(thread->wait_next)
		{
			thread->wait_next->wait_prev = NULL;
		}

	}
	else
	{
		thread->wait_prev->wait_next = thread->wait_next;
		if(thread->wait_next)
		{
			thread->wait_next->wait_prev = thread->wait_prev;
		}
		else
		{
			s->tail = thread->wait_prev;
		}
	}

	if(s->tail == thread)
	{
		s->tail = NULL;
	}


	thread->wait_next = thread->wait_prev = NULL;
}

}

using Scheduler::thread;

#include <carbon/percpu.h>

inline struct thread *get_current_thread()
{
	return get_per_cpu(Scheduler::current_thread);
}

inline struct thread *get_current_for_cpu(unsigned int cpu)
{
	return other_cpu_get(Scheduler::current_thread, cpu);
}

namespace Scheduler
{
	inline void SetCurrentState(unsigned int state)
	{
		get_current_thread()->status = state;
	}
}

#endif