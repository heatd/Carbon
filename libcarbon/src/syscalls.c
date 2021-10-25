/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include "../include/carbon/syscalls.h"
#define _GNU_SOURCE
#include <bits/syscall.h>
#include <unistd.h>
#include <carbon/public/vm.h>

long set_tid_address(int *ptr)
{
	return syscall(SYS_set_tid_address, ptr);
}

cbn_status_t cbn_tls_op(enum tls_op code, unsigned long addr)
{
	/* In reality, it's SYS_cbn_tls_op; TODO: Fix name */
	syscall(SYS_arch_prctl, addr);
}

__attribute__((noreturn))
void cbn_exit_process(int exit_code)
{
	syscall(SYS_cbn_exit_process, exit_code);
	__builtin_unreachable();
}

cbn_status_t cbn_open_sys_handle(const char *upath, unsigned long permitions)
{
	return syscall(SYS_cbn_open_sys_handle, upath, permitions);
}

cbn_status_t cbn_duplicate_handle(cbn_handle_t process_handle, cbn_handle_t srchandle_id,
	cbn_handle_t dsthandle, cbn_handle_t *result, unsigned long flags)
{
	return syscall(SYS_cbn_duplicate_handle, process_handle, srchandle_id, dsthandle, result, flags);
}

cbn_status_t cbn_write(cbn_handle_t handle, const void *buffer, size_t len, size_t *written)
{
	return syscall(SYS_cbn_write, handle, buffer, len, written);
}

cbn_status_t cbn_read(cbn_handle_t handle, void *buffer, size_t len, size_t *read)
{
	return syscall(SYS_cbn_read, handle, buffer, len, read);
}

cbn_status_t cbn_writev(cbn_handle_t handle, const struct iovec *iovs, int veccnt, size_t *res)
{
	return syscall(SYS_cbn_writev, handle, iovs, veccnt, res);
}

cbn_status_t cbn_readv(cbn_handle_t handle, const struct iovec *iovs, int veccnt, size_t *res)
{
	return syscall(SYS_cbn_readv, handle, iovs, veccnt, res);
}

cbn_status_t cbn_vmo_create(size_t size, cbn_handle_t *out)
{
	return syscall(SYS_cbn_vmo_create, size, out);
}

cbn_status_t cbn_mmap(cbn_handle_t process_handle, cbn_handle_t vmo_handle, void *hint,
			     size_t length, size_t off, long flags, long prot, void **result)
{
	struct __cbn_mmap_packed_args args;
	args.flags = flags;
	args.length = length;
	args.off = off;
	/* We need this struct because of the argument limit */

	return syscall(SYS_cbn_mmap, process_handle, vmo_handle, hint, &args, prot, result);
}