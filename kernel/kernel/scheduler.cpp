/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <carbon/scheduler.h>
#include <carbon/cpu.h>
#include <carbon/timer.h>
#include <carbon/percpu.h>

extern struct serial_port com1;
void serial_write(const char *s, size_t size, struct serial_port *port);

namespace Scheduler
{

size_t thread_id = 0;
static bool scheduler_initialized = false;

struct thread *CreateThread(struct registers *regs, CreateThreadFlags flags)
{
	thread *t = new thread;
	if(!t)
		return nullptr;

	memset(t, 0, sizeof(*t));

	t->flags = flags;
	t->status = THREAD_RUNNABLE;
	t->priority = SCHED_PRIO_NORMAL;

	__sync_fetch_and_add(&t->thread_id, 1);
	if(!ArchCreateThreadLowLevel(t, regs))
	{
		delete t;
		return nullptr;
	}

	return t;
}

struct thread *CreateThread(ThreadCallback callback, void *context, CreateThreadFlags flags)
{
	thread *t = new thread;

	if(!t)
		return nullptr;

	memset(t, 0, sizeof(*t));
	t->flags = flags;
	t->status = THREAD_RUNNABLE;
	t->priority = SCHED_PRIO_NORMAL;

	__sync_fetch_and_add(&t->thread_id, 1);
	if(!ArchCreateThread(t, callback, context, flags))
	{
		delete t;
		return nullptr;
	}

	return t;
}

inline unsigned long GetSchedQuantum()
{
	return 10;
}

#define EARLY_BOOT_GDB_DELAY	\
volatile int __gdb_debug_counter = 0; \
while(__gdb_debug_counter != 1)

void IdleThread(void *context)
{
	(void) context;

	while(true)
		__asm__ __volatile__("hlt");
}

void InitializeIdleThread()
{
	auto idle = CreateThread(IdleThread, nullptr, CREATE_THREAD_KERNEL);

	assert(idle != nullptr);

	idle->priority = SCHED_PRIO_VERY_LOW;
	StartThread(idle);
}

PER_CPU_VAR(struct thread *current_thread) = nullptr;
PER_CPU_VAR(unsigned long sched_quantum) = 0;
PER_CPU_VAR(unsigned long preemption_counter) = 0;
PER_CPU_VAR(Spinlock scheduler_lock) = {};

void Initialize()
{
	/* This should be ran with interrupts disabled */

	/* Create the current thread */
	thread *t = new thread;

	assert(t != nullptr);

	memset(t, 0, sizeof(thread));

	t->status = THREAD_RUNNABLE;
	t->flags = THREAD_FLAG_KERNEL;
	t->priority = SCHED_PRIO_NORMAL;

	__sync_fetch_and_add(&t->thread_id, 1);

	/* And set it */
	write_per_cpu(current_thread, t);
	write_per_cpu(sched_quantum, GetSchedQuantum());
	write_per_cpu(preemption_counter, 0);

	scheduler_initialized = true;

	InitializeIdleThread();
}

void OnTick()
{
	add_per_cpu(sched_quantum, -1);

	if(get_per_cpu(sched_quantum) == 0)
	{
		auto current = get_current_thread();
		current->flags |= THREAD_FLAG_NEEDS_RESCHED;
	}
}

bool IsPreemptionDisabled()
{
	return get_per_cpu(preemption_counter) > 0;
}

void DisablePreemptionForCpu(int cpu)
{
	/* TODO: Add the ability to fetch other cpu's vars */
	add_per_cpu(preemption_counter, 1);
}

void DisablePreemption()
{
	if(scheduler_initialized)
		DisablePreemptionForCpu(0);
}

void EnablePreemptionForCpu(int cpu)
{
	assert(get_per_cpu(preemption_counter) != 0);
	add_per_cpu(preemption_counter, -1);
}

void EnablePreemption()
{
	if(scheduler_initialized)
		EnablePreemptionForCpu(0);
}

void SaveThread(struct thread *thread, struct registers *regs)
{
	thread->kernel_stack = (unsigned long *) regs;
	thread->thread_errno = errno;

	/* TODO: Call ArchSaveThread so we can possibly save the x86 fpu */
}

PER_CPU_VAR(struct thread *thread_queues_head[NR_PRIO]);
PER_CPU_VAR(struct thread *thread_queues_tail[NR_PRIO]);

void AppendThreadToQueue(struct thread *thread, unsigned int priority)
{
	assert(get_per_cpu_ptr(scheduler_lock)->IsLocked() == true);

	auto tq_head = (struct thread **) get_per_cpu_ptr_no_cast(thread_queues_head);
	auto tq_tail = (struct thread **) get_per_cpu_ptr_no_cast(thread_queues_tail);

	if(!tq_head[priority])
	{
		tq_head[priority] = tq_tail[priority] = thread;
	}
	else
	{
		tq_tail[priority]->next = thread;
		thread->prev = tq_tail[priority];
		tq_tail[priority] = thread;
	}
}

struct thread *ScheduleThreadForCpu(struct thread *current)
{
	ScopedSpinlockIrqsave guard{get_per_cpu_ptr(scheduler_lock)};

	if(current->status == THREAD_RUNNABLE)
	{
		AppendThreadToQueue(current, current->priority);
	}

	auto tq_head = (struct thread **) get_per_cpu_ptr_no_cast(thread_queues_head);
	auto tq_tail = (struct thread **) get_per_cpu_ptr_no_cast(thread_queues_tail);

	for(int i = NR_PRIO-1; i >= 0; i--)
	{
		/* If this queue has a thread, we found a runnable thread! */
		if(tq_head[i])
		{
			struct thread *ret = tq_head[i];
			
			/* Advance the queue by one */
			tq_head[i] = ret->next;
			if(tq_head[i])
				tq_head[i]->prev = nullptr;
			if(tq_tail[i] == ret)
				tq_tail[i] = nullptr;
			ret->next = nullptr;

			return ret;
		}
	}

	return nullptr;
}

struct thread *ScheduleThread(struct thread *current)
{
	return ScheduleThreadForCpu(current);
}

struct registers *Schedule(struct registers *regs)
{
	auto current = get_current_thread();
	SaveThread(current, regs);

	if(IsPreemptionDisabled())
	{
		write_per_cpu(sched_quantum, GetSchedQuantum() / 2);
		return regs;
	}

	current->flags &= ~(THREAD_FLAG_NEEDS_RESCHED);

	auto new_thread = ScheduleThread(current);

	write_per_cpu(current_thread, new_thread);
	assert(new_thread->kernel_stack != nullptr);

	write_per_cpu(sched_quantum, GetSchedQuantum());

	return (struct registers *) new_thread->kernel_stack;
}

/* To use in ASM */
extern "C"
struct registers *schedule(struct registers *regs)
{
	return Schedule(regs);
}

void SetThreadAsRunnable(struct thread *thread)
{
	thread->status = THREAD_RUNNABLE;
	thread->cpu = 0;

	ScopedSpinlockIrqsave guard {get_per_cpu_ptr(scheduler_lock)};
	Scheduler::AppendThreadToQueue(thread, thread->priority);
}

void StartThread(struct thread *thread)
{
	/* TODO: Allocate CPU when we add SMP */
	SetThreadAsRunnable(thread);
}

extern "C" void platform_yield();
void Yield()
{
	platform_yield();
}

void Block(struct thread *t)
{
	t->status = THREAD_BLOCKED;
	Yield();
}

void Sleep(ClockSource::ClockTicks ticks)
{
	auto thread = get_current_thread();
	Timer::TimerEvent wake_event(ticks, [] (void *context)
	{
		/* TODO: Get actual CPU when we add SMP */

		ScopedSpinlockIrqsave guard {get_per_cpu_ptr(scheduler_lock)};
		struct thread *thread = (struct thread *) context;
		
		thread->status = THREAD_RUNNABLE;
		AppendThreadToQueue(thread, thread->priority);
	}
	, (void *) thread, true);

	Timer::AddTimerEvent(wake_event);

	Block(thread);
}

}