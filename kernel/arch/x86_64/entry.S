/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

.section .text

.macro pushaq
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rdi
	push %rsi
	push %rbp
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
.endm

.macro popaq
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rsi
	pop %rdi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
.endm

.global isr_common
isr_common:
	cld
	mov %ds, %rax
	push %rax
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov 152(%rsp), %rax
	cmp $0x8, %rax
	je .skip_swap
	swapgs
.skip_swap: 
	mov %rsp, %rdi
	call x86_exception_gate
.isr_continue:
	mov 152(%rsp), %rax
	cmp $0x8, %rax
	je .skip_swap2
	swapgs
.skip_swap2:
	pop %rax
	mov %ax, %ds
	mov %ax, %es
	popaq
	add $16, %rsp
	iretq

.macro ISR_NOERRCODE isr_num
.global isr\isr_num
isr\isr_num:
	cli
	pushq 0
	pushq $\isr_num
	pushaq
	jmp isr_common
.endm

.macro ISR_ERRCODE isr_num
.global isr\isr_num
isr\isr_num:
	cli
	pushq $\isr_num
	pushaq
	jmp isr_common
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

.global idt_flush
idt_flush:
	lidt (%rdi)
	ret