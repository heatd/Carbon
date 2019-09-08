/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <carbon/scheduler.h>
#include <carbon/cpu.h>
#include <carbon/timer.h>
#include <carbon/percpu.h>
#include <carbon/smp.h>
#include <carbon/fpu.h>
#include <carbon/mutex.h>
#include <carbon/rwlock.h>
#include <carbon/atomic.h>

extern struct serial_port com1;
void serial_write(const char *s, size_t size, struct serial_port *port);

namespace scheduler
{

atomic<size_t> thread_id{0};

static bool scheduler_initialized = false;

bool create_fpu_area(struct thread *t)
{
	if(t->flags & THREAD_FLAG_KERNEL)
	{
		t->fpu_area = nullptr;
		return true;
	}

	/* We need to use memalign here because of the alignment requirement */
	auto fpu_area = (unsigned char *) memalign(FPU_AREA_ALIGNMENT, FPU_AREA_SIZE);
	if(!(t->fpu_area = fpu_area))
		return false;
	memset(fpu_area, 0, FPU_AREA_SIZE);

	return true;
}

struct thread *create_thread(struct registers *regs, create_thread_flags flags)
{
	thread *t = new thread;
	if(!t)
		return nullptr;

	memset(t, 0, sizeof(*t));

	t->flags = flags;
	t->status = THREAD_BLOCKED;
	t->priority = SCHED_PRIO_NORMAL;

	if(!create_fpu_area(t))
	{
		delete t;
		return nullptr;
	}

	t->thread_id = thread_id++;
	if(!arch_create_thread_low_level(t, regs))
	{
		if(t->fpu_area)
			free(t->fpu_area);
		delete t;
		return nullptr;
	}

	return t;
}

struct thread *create_thread(thread_callback callback, void *context, create_thread_flags flags)
{
	thread *t = new thread;

	if(!t)
		return nullptr;

	memset(t, 0, sizeof(*t));
	t->flags = flags;
	t->status = THREAD_BLOCKED;
	t->priority = SCHED_PRIO_NORMAL;

	if(!create_fpu_area(t))
	{
		delete t;
		return nullptr;
	}

	t->thread_id = thread_id++;

	if(!arch_create_thread(t, callback, context, flags))
	{
		if(t->fpu_area)
			free(t->fpu_area);
		delete t;
		return nullptr;
	}

	return t;
}

inline unsigned long get_sched_quantum()
{
	return 10;
}

#define EARLY_BOOT_GDB_DELAY	\
volatile int __gdb_debug_counter = 0; \
while(__gdb_debug_counter != 1)

void idle_thread(void *context)
{
	(void) context;

	while(true)
		__asm__ __volatile__("hlt");
}

void initialize_idle_thread()
{
	auto idle = create_thread(idle_thread, nullptr, CREATE_THREAD_KERNEL);

	assert(idle != nullptr);

	idle->priority = SCHED_PRIO_VERY_LOW;
	start_thread(idle, 0);
}

PER_CPU_VAR(struct thread *current_thread);
PER_CPU_VAR(unsigned long sched_quantum);
PER_CPU_VAR(unsigned long preemption_counter);
PER_CPU_VAR(Spinlock scheduler_lock);
PER_CPU_VAR(unsigned int cpu_load);

void initialize()
{
	/* This should be ran with interrupts disabled */

	/* Create the current thread */
	thread *t = new thread;

	assert(t != nullptr);

	memset(t, 0, sizeof(thread));

	t->status = THREAD_RUNNABLE;
	t->flags = THREAD_FLAG_KERNEL;
	t->priority = SCHED_PRIO_NORMAL;

	t->thread_id = thread_id++;
	/* And set it */
	write_per_cpu(current_thread, t);
	write_per_cpu(sched_quantum, get_sched_quantum());
	write_per_cpu(preemption_counter, 0);
	add_per_cpu(cpu_load, 1);

	scheduler_initialized = true;

	initialize_idle_thread();
}

void on_tick()
{
	add_per_cpu(sched_quantum, -1);

	if(get_per_cpu(sched_quantum) == 0)
	{
		auto current = get_current_thread();
		current->flags |= THREAD_FLAG_NEEDS_RESCHED;
	}
}

bool is_preemption_disabled()
{
	return get_per_cpu(preemption_counter) > 0;
}

void disable_preemption_for_cpu(unsigned int cpu)
{
	add_per_cpu_any(preemption_counter, 1, cpu);
}

void disable_preemption()
{
	if(scheduler_initialized)
		disable_preemption_for_cpu(get_cpu_nr());
}

void enable_preemption_for_cpu(unsigned int cpu)
{
	assert(get_per_cpu_any(preemption_counter, cpu) != 0);
	add_per_cpu_any(preemption_counter, -1, cpu);
}

void enable_preemption()
{
	if(scheduler_initialized)
		enable_preemption_for_cpu(get_cpu_nr());
}

void save_thread(struct thread *thread, struct registers *regs)
{
	thread->kernel_stack = (unsigned long *) regs;
	thread->thread_errno = errno;

	arch_save_thread(thread);
}

PER_CPU_VAR(struct thread *thread_queues_head[NR_PRIO]);
PER_CPU_VAR(struct thread *thread_queues_tail[NR_PRIO]);

void append_thread_to_queue(struct thread *thread, unsigned int priority)
{
	auto cpu = thread->cpu;
	assert(get_per_cpu_ptr_any(scheduler_lock, cpu)->IsLocked() == true);

	auto tq_head = (struct thread **) get_per_cpu_ptr_any(thread_queues_head, cpu);
	auto tq_tail = (struct thread **) get_per_cpu_ptr_any(thread_queues_tail, cpu);

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

struct thread *schedule_thread_for_cpu(struct thread *current)
{
	scoped_spinlock_irqsave guard{get_per_cpu_ptr(scheduler_lock)};

	if(current->status == THREAD_RUNNABLE)
	{
		append_thread_to_queue(current, current->priority);
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

struct thread *schedule_thread(struct thread *current)
{
	return schedule_thread_for_cpu(current);
}

struct registers *schedule(struct registers *regs)
{
	auto current = get_current_thread();
	save_thread(current, regs);

	if(is_preemption_disabled())
	{
		write_per_cpu(sched_quantum, get_sched_quantum() / 2);
		return regs;
	}

	current->flags &= ~(THREAD_FLAG_NEEDS_RESCHED);

	auto new_thread = schedule_thread(current);

	write_per_cpu(current_thread, new_thread);
	assert(new_thread->kernel_stack != nullptr);

	write_per_cpu(sched_quantum, get_sched_quantum());

	arch_load_thread(new_thread);

	return (struct registers *) new_thread->kernel_stack;
}

/* To use in ASM */
extern "C"
struct registers *asm_schedule(struct registers *regs)
{
	return schedule(regs);
}

void set_thread_as_runnable(struct thread *thread)
{
	thread->status = THREAD_RUNNABLE;

	scoped_spinlock_irqsave guard {get_per_cpu_ptr_any(scheduler_lock, thread->cpu)};
	scheduler::append_thread_to_queue(thread, thread->priority);
}

struct registers *thread::get_registers()
{
	/* Inspecting a running/runnable thread's status is incorrect */
	assert(status != THREAD_RUNNABLE);

	/* TODO: This is at least true for x86, maybe not for other archs? */
	return (struct registers *) kernel_stack;
}

unsigned int allocate_cpu()
{
	unsigned int min_load = UINT_MAX;
	unsigned int target_cpu = 0;

	for(unsigned int i = 0; i < Smp::GetOnlineCpus(); i++)
	{
		auto this_cpu_load = get_per_cpu_any(cpu_load, i);

		if(this_cpu_load < min_load)
		{
			min_load = this_cpu_load;
			target_cpu = i;
		}
	}

	add_per_cpu_any(cpu_load, 1, target_cpu);

	printf("Allocated cpu%u\n", target_cpu);

	return target_cpu;
}

void start_thread(struct thread *thread, unsigned int cpu)
{
	if(cpu != (unsigned int) -1)
		thread->cpu = cpu;
	else
		thread->cpu = scheduler::allocate_cpu();
	set_thread_as_runnable(thread);
}

extern "C" void platform_yield();
void yield()
{
	platform_yield();
}

void block(struct thread *t)
{
	t->status = THREAD_BLOCKED;
	yield();
}

void unblock_thread(struct thread *thread)
{
	scoped_spinlock_irqsave guard {get_per_cpu_ptr_any(scheduler_lock, thread->cpu)};

	auto cpu = thread->cpu;

	if(thread->status != THREAD_BLOCKED)
		return;

	thread->status = THREAD_RUNNABLE;

	if(get_current_for_cpu(cpu) == thread)
	{
		/* Just set thread->status and return */
		return;
	}
	else
	{
		append_thread_to_queue(thread, thread->priority);
	}
}

void sleep(ClockSource::ClockTicks ticks)
{
	auto thread = get_current_thread();
	Timer::TimerEvent wake_event(ticks, [] (void *context)
	{
		struct thread *thread = (struct thread *) context;
		unblock_thread(thread);
	}
	, (void *) thread, true);

	Timer::AddTimerEvent(wake_event);

	block(thread);
}

void setup_cpu(unsigned int cpu)
{
	auto thread = create_thread(idle_thread, nullptr, CREATE_THREAD_KERNEL);
	assert(thread != nullptr);
	thread->cpu = cpu;
	thread->priority = 0;
	
	thread->status = THREAD_RUNNABLE;

	write_per_cpu_any(current_thread, thread, cpu);
	write_per_cpu_any(sched_quantum, get_sched_quantum(), cpu);
	write_per_cpu_any(preemption_counter, 0, cpu);
}

}

void mutex_lock_slow_path(struct raw_mutex *mutex)
{
	spin_lock(&mutex->llock);

	scheduler::set_current_state(THREAD_BLOCKED);

	while(!__sync_bool_compare_and_swap(&mutex->counter, 0, 1))
	{
		struct thread *thread = get_current_thread();

		scheduler::enqueue_thread(mutex, thread);
		
		spin_unlock(&mutex->llock);

		scheduler::yield();

		spin_lock(&mutex->llock);

		scheduler::set_current_state(THREAD_BLOCKED);
	}

	scheduler::set_current_state(THREAD_RUNNABLE);

	spin_unlock(&mutex->llock);

	__sync_synchronize();
}

void mutex_lock(struct raw_mutex *mtx)
{
	if(!__sync_bool_compare_and_swap(&mtx->counter, 0, 1))
		mutex_lock_slow_path(mtx);

	__sync_synchronize();
	mtx->owner = get_current_thread();
}

void mutex_unlock(struct raw_mutex *mtx)
{
	mtx->owner = nullptr;
	__sync_bool_compare_and_swap(&mtx->counter, 1, 0);
	__sync_synchronize();

	spin_lock(&mtx->llock);

	if(mtx->head)
	{
		struct thread *t = mtx->head;
		scheduler::dequeue_thread(mtx, mtx->head);

		scheduler::unblock_thread(t);
	}

	spin_unlock(&mtx->llock);
}

bool rw_lock::trylock_read()
{
	unsigned long c;
	do
	{
		c = counter;
		if(c == max_readers)
			return false;
		if(c == counter_locked_write)
			return false;
	} while(__sync_bool_compare_and_swap(&counter, c, c + 1) != true);
	
	__sync_synchronize();

	return true;
}

bool rw_lock::trylock_write()
{
	bool st = __sync_bool_compare_and_swap(&counter, 0, counter_locked_write);
	__sync_synchronize();
	
	return st;
}

void rw_lock::lock_read()
{
	llock.Lock();

	scheduler::set_current_state(THREAD_BLOCKED);

	while(!trylock_read())
	{
		struct thread *thread = get_current_thread();

		scheduler::enqueue_thread(this, thread);
		
		llock.Unlock();

		scheduler::yield();

		llock.Lock();

		scheduler::set_current_state(THREAD_BLOCKED);
	}

	scheduler::set_current_state(THREAD_RUNNABLE);

	llock.Unlock();

	__sync_synchronize();
}

void rw_lock::lock_write()
{
	llock.Lock();

	scheduler::set_current_state(THREAD_BLOCKED);

	while(!trylock_write())
	{
		struct thread *thread = get_current_thread();

		scheduler::enqueue_thread(this, thread);
		
		llock.Unlock();

		scheduler::yield();

		llock.Lock();

		scheduler::set_current_state(THREAD_BLOCKED);
	}

	scheduler::set_current_state(THREAD_RUNNABLE);

	llock.Unlock();

	__sync_synchronize();
}

void rw_lock::wake_up_threads()
{
	llock.Lock();

	while(head)
	{
		auto to_wake = head;
		scheduler::dequeue_thread(this, head);

		scheduler::unblock_thread(to_wake);
	}

	llock.Unlock();
}

void rw_lock::wake_up_thread()
{
	llock.Lock();

	if(head)
	{
		auto to_wake = head;
		scheduler::dequeue_thread(this, head);

		scheduler::unblock_thread(to_wake);
	}

	llock.Unlock();
}

void rw_lock::unlock_read()
{
	/* Implementation note: If we're unlocking a read lock, only wake up a
	 * single thread, since the write lock is exclusive, like a mutex.
	*/
	if(__sync_sub_and_fetch(&counter, 1) == 0)
		wake_up_thread();
}

void rw_lock::unlock_write()
{
	counter = 0;
	__sync_synchronize();
	/* Implementation note: If we're unlocking a write lock, wake up every single thread
	 * because we can have both readers and writers waiting to get woken up.
	*/
	wake_up_threads();
}

#ifdef MUTEX_TEST
struct mutex mtx = {};

void mtx_thread2(void *c)
{
	unsigned int cpu = (unsigned int) (unsigned long) c;

	while(true)
	{
		mutex_lock(&mtx);
		printf("thread%u\n", cpu);
		mutex_unlock(&mtx);
	}
}

thread *threads[4];
void mtx_test()
{
	for(unsigned int i = 1; i < Smp::GetOnlineCpus(); i++)
	{
		printf("create thread %u\n", i);
		auto t = scheduler::create_thread(mtx_thread2, (void *) (unsigned long) i,
					 scheduler::CreateThreadFlags::CREATE_THREAD_KERNEL);
		threads[i] = t;
		scheduler::StartThread(t, i);
	}

	while(true)
	{
		mutex_lock(&mtx);
		printf("thread0\n");
		mutex_unlock(&mtx);
	}
}

#endif

#ifdef WAITQUEUE_TEST
#include <carbon/wait_queue.h>

thread *threads[4];
WaitQueue wq;

void wq_thread(void *context)
{
	auto cpu = (unsigned int) (unsigned long) context;

	wq.AcquireLock();
	while(true)
	{
		wq.Wait();
		printf("cpu%u woke up\n", cpu);
	}
}

void wq_test()
{
	for(unsigned int i = 1; i < Smp::GetOnlineCpus(); i++)
	{
		printf("create thread %u\n", i);
		auto t = scheduler::create_thread(wq_thread, (void *) (unsigned long) i,
					 scheduler::CreateThreadFlags::CREATE_THREAD_KERNEL);
		threads[i] = t;
		scheduler::StartThread(t, i);
	}

	while(true)
	{
		wq.WakeUpAll();
	}
}

#endif