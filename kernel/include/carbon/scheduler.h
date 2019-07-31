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

namespace scheduler
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

using thread_callback = void (*)(void *context);

enum create_thread_flags : unsigned int
{
	CREATE_THREAD_KERNEL = THREAD_FLAG_KERNEL
};

struct thread *create_thread(thread_callback callback, void *context,
			    create_thread_flags flags);
struct thread *create_thread(struct registers *regs, create_thread_flags flags);
void start_thread(struct thread *thread, unsigned int cpu = -1);

bool arch_create_thread(struct thread *thread, thread_callback callback,
		      void *context, create_thread_flags flags);
bool arch_create_thread_low_level(struct thread *thread, struct registers *regs);


void arch_load_thread(struct thread *thread);

void arch_save_thread(struct thread *thread);

void on_tick();

void initialize();

struct registers *schedule(struct registers *regs);

void disable_preemption();
void enable_preemption();

void yield();

void sleep(ClockSource::ClockTicks ticks);

void setup_cpu(unsigned int cpu);

void unblock_thread(struct thread *thread);

void block(struct thread *t);

extern struct thread *current_thread;

template <typename T>
void enqueue_thread(T *s, struct thread *thread)
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
void dequeue_thread(T *s, struct thread *thread)
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

using scheduler::thread;

#include <carbon/percpu.h>

inline struct thread *get_current_thread()
{
	return get_per_cpu(scheduler::current_thread);
}

inline struct thread *get_current_for_cpu(unsigned int cpu)
{
	return other_cpu_get(scheduler::current_thread, cpu);
}

namespace scheduler
{
	inline void set_current_state(unsigned int state)
	{
		get_current_thread()->status = state;
	}
}

#endif