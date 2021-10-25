/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/x86/msr.h>
#include <carbon/x86/eflags.h>
#include <carbon/x86/segments.h>
#include <carbon/x86/syscall.h>
#include <carbon/carbon.h>
#include <carbon/status.h>
#include <carbon/public/tlsop.h>
#include <carbon/public/handle.h>

#include <stdio.h>
#include <sys/uio.h>

long sys_set_tid_address(int *ptr);
cbn_status_t sys_cbn_tls_op(tls_op code, unsigned long addr);
__attribute__((noreturn))
void sys_cbn_exit_process(int exit_code);
cbn_status_t sys_cbn_open_sys_handle(const char *upath, unsigned long permitions);
cbn_status_t sys_cbn_duplicate_handle(cbn_handle_t process_handle, cbn_handle_t srchandle_id,
	cbn_handle_t dsthandle, cbn_handle_t *result, unsigned long flags);
cbn_status_t sys_cbn_write(cbn_handle_t handle, const void *buffer, size_t len, size_t *written);
cbn_status_t sys_cbn_read(cbn_handle_t handle, void *buffer, size_t len, size_t *read);
cbn_status_t sys_cbn_writev(cbn_handle_t handle, const struct iovec *iovs, int veccnt, size_t *res);
cbn_status_t sys_cbn_readv(cbn_handle_t handle, const struct iovec *iovs, int veccnt, size_t *res);
cbn_status_t sys_cbn_vmo_create(size_t size, cbn_handle_t *out);
cbn_status_t sys_cbn_mmap(cbn_handle_t process_handle, cbn_handle_t vmo_handle, void *hint,
			    struct __cbn_mmap_packed_args *packed_args, long prot, void **result);
cbn_status_t sys_cbn_unmap(cbn_handle_t process_handle, void *ptr, size_t length);

namespace x86
{

namespace syscall
{

extern "C" void syscall_entry64();

void init_syscall()
{
	wrmsr(IA32_MSR_STAR, ((unsigned long) (((USER32_CS | X86_USER_MODE_FLAG) << 16) | KERNEL_CS)) << 32);
	wrmsr(IA32_MSR_LSTAR, (unsigned long) &syscall_entry64);
	wrmsr(IA32_MSR_SFMASK, EFLAGS_INT_ENABLED | EFLAGS_DIRECTION | EFLAGS_TRAP | EFLAGS_ALIGNMENT_CHECK);
}

typedef long (*syscall_callback_t)(unsigned long rdi, unsigned long rsi,
				   unsigned long rdx, unsigned long r10,
				   unsigned long r8, unsigned long r9);

long sys_badsys()
{
	return CBN_STATUS_BADSYS;
}

void *syscall_table_64[] = 
{
	(void *) sys_set_tid_address,
	(void *) sys_cbn_tls_op,
	(void *) sys_cbn_exit_process,
	(void *) sys_cbn_read,
	(void *) sys_cbn_write,
	(void *) sys_badsys,
	(void *) sys_badsys,
	(void *) sys_cbn_open_sys_handle,
	(void *) sys_cbn_readv,
	(void *) sys_cbn_writev,
	(void *) sys_cbn_duplicate_handle,
	(void *) sys_cbn_vmo_create,
	(void *) sys_cbn_mmap
};

extern "C" long do_syscall64(struct syscall_frame *frame)
{
	long syscall_nr = frame->rax;
	long ret = 0;

	//proc_event_enter_syscall(frame, frame->rax);
	//printf("syscall start %lu\n", syscall_nr);
	if(likely(syscall_nr <= NR_SYSCALL_MAX))
	{
		ret = ((syscall_callback_t) syscall_table_64[syscall_nr])(frame->rdi, frame->rsi,
						   frame->rdx, frame->r10,
						   frame->r8, frame->r9);
	}
	else
		ret = CBN_STATUS_BADSYS;	

	//proc_event_exit_syscall(ret, syscall_nr);
	//printf("System call %lu\n", syscall_nr);

	return ret;
}

}

}
