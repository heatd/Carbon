/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

.section .text

#include "register_ops.S"
#include <carbon/x86/segments.h>

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
	pushq $0
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

#define IRQ_STACK_OFF_CS		136
#define KERNEL_CS			0x08
.global irq_common
irq_common:
	cld
	mov %ds, %ax
	push %rax
	mov $0x10, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %es

	mov IRQ_STACK_OFF_CS(%rsp), %rax
	cmp $KERNEL_CS, %rax
	je .L3
	swapgs
.L3:
	push %rdi
	call platform_send_eoi
	pop %rdi
	mov %rsp, %rsi
	# End the stack frame list so we stop here
	xor %rbp, %rbp
	call IrqEntry
	mov %rax, %rsp
	call signal_is_pending
	cmp $1, %rax
	je .L4
.global x86_scheduler_exit
x86_scheduler_exit:
	/* We'll use this bit of code as a trampoline to the new thread too */
.L5:
	mov IRQ_STACK_OFF_CS(%rsp), %rax
	cmp $KERNEL_CS, %rax
	je .L6
	swapgs
.L6:
	pop %rax
	mov %ax, %ds
	mov %ax, %es

	popaq
	iretq

.L4:
	mov %rsp, %rdi
	mov $0, %rsi
	call handle_signal
	jmp .L5

.macro IRQ irq_num
.global irq\irq_num
irq\irq_num:
	cli
	pushaq
	mov $\irq_num, %rdi
	jmp irq_common

.global irq\irq_num\()_end
irq\irq_num\()_end:

.endm

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15
IRQ 16
IRQ 17
IRQ 18
IRQ 19
IRQ 20
IRQ 21
IRQ 22
IRQ 23
IRQ 24
IRQ 25
IRQ 26
IRQ 27
IRQ 28
IRQ 29
IRQ 30
IRQ 31
IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47
IRQ 48
IRQ 49
IRQ 50
IRQ 51
IRQ 52
IRQ 53
IRQ 54
IRQ 55
IRQ 56
IRQ 57
IRQ 58
IRQ 59
IRQ 60
IRQ 61
IRQ 62
IRQ 63
IRQ 64
IRQ 65
IRQ 66
IRQ 67
IRQ 68
IRQ 69
IRQ 70
IRQ 71
IRQ 72
IRQ 73
IRQ 74
IRQ 75
IRQ 76
IRQ 77
IRQ 78
IRQ 79
IRQ 80
IRQ 81
IRQ 82
IRQ 83
IRQ 84
IRQ 85
IRQ 86
IRQ 87
IRQ 88
IRQ 89
IRQ 90
IRQ 91
IRQ 92
IRQ 93
IRQ 94
IRQ 95
IRQ 96
IRQ 97
IRQ 98
IRQ 99
IRQ 100
IRQ 101
IRQ 102
IRQ 103
IRQ 104
IRQ 105
IRQ 106
IRQ 107
IRQ 108
IRQ 109
IRQ 110
IRQ 111
IRQ 112
IRQ 113
IRQ 114
IRQ 115
IRQ 116
IRQ 117
IRQ 118
IRQ 119
IRQ 120
IRQ 121
IRQ 122
IRQ 123
IRQ 124
IRQ 125
IRQ 126
IRQ 127
IRQ 128
IRQ 131
IRQ 132
IRQ 133
IRQ 134
IRQ 135
IRQ 136
IRQ 137
IRQ 138
IRQ 139
IRQ 140
IRQ 141
IRQ 142
IRQ 143
IRQ 144
IRQ 145
IRQ 146
IRQ 147
IRQ 148
IRQ 149
IRQ 150
IRQ 151
IRQ 152
IRQ 153
IRQ 154
IRQ 155
IRQ 156
IRQ 157
IRQ 158
IRQ 159
IRQ 160
IRQ 161
IRQ 162
IRQ 163
IRQ 164
IRQ 165
IRQ 166
IRQ 167
IRQ 168
IRQ 169
IRQ 170
IRQ 171
IRQ 172
IRQ 173
IRQ 174
IRQ 175
IRQ 176
IRQ 177
IRQ 178
IRQ 179
IRQ 180
IRQ 181
IRQ 182
IRQ 183
IRQ 184
IRQ 185
IRQ 186
IRQ 187
IRQ 188
IRQ 189
IRQ 190
IRQ 191
IRQ 192
IRQ 193
IRQ 194
IRQ 195
IRQ 196
IRQ 197
IRQ 198
IRQ 199
IRQ 200
IRQ 201
IRQ 202
IRQ 203
IRQ 204
IRQ 205
IRQ 206
IRQ 207
IRQ 208
IRQ 209
IRQ 210
IRQ 211
IRQ 212
IRQ 213
IRQ 214
IRQ 215
IRQ 216
IRQ 217
IRQ 218
IRQ 219
IRQ 220
IRQ 221

# Syscall ABI details:
# Arguments are passed on %rdi, %rsi, %rdx, %r10, %r8 and %r9
# Syscall number passed in %rax
# Return address is stored in %rcx by the syscall instruction
# RFLAGS stored in r11
# Registers %rbx, %rsp, %rbp, %r12-15 are preserved
# Registers %rax, %rcx, %r11 are clobbered

.macro SAVE_PRESERVED_SYSCALL_REGS
	# First get the user stack
	mov %gs:scratch_rsp, %r11

	# Then push the user %rsp
	push %r11

	# Now everything else
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15
.endm

.macro RESTORE_PRESERVED_SYSCALL_REGS
	# Undo what we did before
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx

	pop %rdi

.endm

.macro CLEAR_CLOBBERED_REGS_EXCEPT_RAX_SYSCALL_RDI
	xor %rsi, %rsi
	xor %rdx, %rdx
	xor %r8, %r8
	xor %r9, %r9
	xor %r10, %r10
.endm

.global syscall_entry64
syscall_entry64:

	.cfi_startproc
	.cfi_register rip, rcx
	.cfi_return_column rcx
	.cfi_def_cfa_register rsp
	.cfi_def_cfa_offset 0

	swapgs

	# We can't preempt right now(unless we want chaos)!
	# Get the kernel stack, and save the user-stack
	mov %rsp, %gs:scratch_rsp
	mov %gs:kernel_stack, %rsp

	# Save %rcx and %r11, since they're needed for sysret
	push %rcx
	push %r11
	
	# Set up the syscall frame
	push %rax
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r8
	push %r9

	# Save the preserved registers
	SAVE_PRESERVED_SYSCALL_REGS

	# End the stack frame list
	xor %rbp, %rbp

	sti
	
	# Ah, we've saved the registers, and the user stack is safe - we can
	# continue now (possible to preempt)
	# Save the old segments, and switch to the kernel ones
	
	mov %ds, %ecx
	push %rcx
	mov $KERNEL_DS, %cx
	mov %cx, %ds
	mov %cx, %es

	mov %rsp, %rdi

	call do_syscall64

	cli

	pop %rcx
	mov %cx, %ds
	mov %cx, %es

	RESTORE_PRESERVED_SYSCALL_REGS

	# and restore the other regs
	pop %r9
	pop %r8
	pop %r10
	pop %rdx
	pop %rsi

	# Save the user rsp really quickly, since no one will touch the stack
	push %rdi
	add $8, %rsp
	# Pop the real %rdi
	pop %rdi

	# Skip %rax
	add $8, %rsp

	# Restore r11 + rcx for sysret
	pop %r11
	pop %rcx

	# Now get the stack
	mov -40(%rsp), %rsp
	
	# and finally, swap back to the user gs
	swapgs

	sysretq

	.cfi_endproc