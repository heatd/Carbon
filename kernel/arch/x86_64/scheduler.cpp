/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stddef.h>
#include <string.h>

#include <carbon/scheduler.h>
#include <carbon/panic.h>
#include <carbon/vm.h>

#include <carbon/fpu.h>
#include <carbon/x86/tss.h>
#include <carbon/memory.h>
#include <carbon/x86/vm.h>

PER_CPU_VAR(unsigned long kernel_stack) = 0;

namespace scheduler
{

constexpr size_t kernel_stack_size = 0x2000;

bool arch_create_thread(struct thread *thread, thread_callback callback,
		      void *context, create_thread_flags flags)
{
	unsigned int cs = 0x2b, ds = 0x33;

	struct registers regs = {};
	regs.rip = (unsigned long) callback;
	/* Arguments are delivered in %rdi */
	regs.rdi = (unsigned long) context;

	if(flags & CREATE_THREAD_KERNEL)
	{
		cs = 0x08;
		ds = 0x10;
	}
	else
	{
		void *user_stack = vm_allocate_region(&kernel_address_space, 0, 0x2000);
		regs.rsp = (unsigned long) user_stack + 0x2000;
	}

	regs.cs = cs;
	regs.ds = ds;

	return arch_create_thread_low_level(thread, &regs);
}

bool arch_create_thread_low_level(struct thread *thread, struct registers *regs)
{
	unsigned long *kernel_stack = (unsigned long *) Vm::mmap(&kernel_address_space, 0,
				kernel_stack_size, VM_PROT_WRITE);

	if(!kernel_stack)
		return false;

	/* Reach the top of the stack */
	thread->kernel_stack_top = kernel_stack =
		(unsigned long *) ((unsigned char *) kernel_stack + kernel_stack_size);
	
	struct registers *stack_regs = ((struct registers *) kernel_stack) - 1;
	memcpy(stack_regs, regs, sizeof(struct registers));

	stack_regs->rflags = 0x202;
	if(thread->flags & THREAD_FLAG_KERNEL)
		stack_regs->rsp = (unsigned long) kernel_stack;
	
	stack_regs->ss = stack_regs->ds;

	thread->kernel_stack = (unsigned long *) stack_regs;

	return true;
}

void arch_save_thread(struct thread *thread)
{
	if(!(thread->flags & THREAD_FLAG_KERNEL))
		Fpu::SaveFpu(thread->fpu_area);
}

void arch_load_thread(struct thread *thread)
{
	write_per_cpu(kernel_stack, (unsigned long) thread->kernel_stack_top);

	Tss::SetKernelStack((unsigned long) thread->kernel_stack_top);

	if(!(thread->flags & THREAD_FLAG_KERNEL))
	{
		Fpu::RestoreFpu(thread->fpu_area);
		vm::switch_address_space(thread->owner->address_space.arch_priv);
	}
}

};