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
	CREATE_THREAD_KERNEL = (1 << 0)
};

struct thread *CreateThread(ThreadCallback callback, void *context,
			    CreateThreadFlags flags);
struct thread *CreateThread(struct registers *regs, CreateThreadFlags flags);
void StartThread(struct thread *thread);

bool ArchCreateThread(struct thread *thread, ThreadCallback callback,
		      void *context, CreateThreadFlags flags);
bool ArchCreateThreadLowLevel(struct thread *thread, struct registers *regs);

void OnTick();

void Initialize();

struct registers *Schedule(struct registers *regs);

void DisablePreemption();
void EnablePreemption();

void Yield();

void Sleep(ClockSource::ClockTicks ticks);

extern struct thread *current_thread;
}

using Scheduler::thread;

#include <carbon/percpu.h>

inline struct thread *get_current_thread()
{
	return get_per_cpu(Scheduler::current_thread);
}

#endif