/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

.section .text

#include "register_ops.S"
#include <carbon/x86/segments.h>

.global platform_yield
platform_yield:
	/* Basically we need to set up an IRQ frame on the stack.
	 * For future reference consult include/carbon/x86/registers.h
	*/
	/* Set up a regular stack frame first */
	push %rbp
	mov %rsp, %rbp
	push %rax	/* We're going to need rax as a scratch register */
	push $KERNEL_DS /* %ss */
	mov %rsp, %rax
	add $8, %rax
	push %rax /* %rsp */
	pushf /* %rflags */
	push $KERNEL_CS /* %cs */
	push $.return_label /* %rip */
	pushaq /* General purpose registers (%rax - %r15) */
	push $KERNEL_DS /* %ds */
	mov %rsp, %rdi
	call schedule
	mov %rax, %rsp
	jmp x86_scheduler_exit
.return_label:
	pop %rax
	pop %rbp
	ret
	