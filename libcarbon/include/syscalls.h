/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _LIBCARBON_SYSCALLS_H
#define _LIBCARBON_SYSCALLH_H

#include <carbon/public/tlsop.h>
#include <carbon/public/handle.h>

#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

long set_tid_address(int *ptr);
cbn_status_t cbn_tls_op(enum tls_op code, unsigned long addr);
__attribute__((noreturn))
void cbn_exit_process(int exit_code);
cbn_status_t cbn_open_sys_handle(const char *upath, unsigned long permitions);
cbn_status_t cbn_duplicate_handle(cbn_handle_t process_handle, cbn_handle_t srchandle_id,
	cbn_handle_t dsthandle, cbn_handle_t *result, unsigned long flags);
cbn_status_t cbn_write(cbn_handle_t handle, const void *buffer, size_t len, size_t *written);
cbn_status_t cbn_read(cbn_handle_t handle, void *buffer, size_t len, size_t *read);
cbn_status_t cbn_writev(cbn_handle_t handle, const struct iovec *iovs, int veccnt, size_t *res);
cbn_status_t cbn_readv(cbn_handle_t handle, const struct iovec *iovs, int veccnt, size_t *res);

#ifdef __cplusplus
}
#endif

#endif