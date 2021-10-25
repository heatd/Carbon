/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#define SMP_TRAMPOLINE_BASE	0x0
#define KERNEL_VIRTUAL_BASE 	0xffffffff80000000
.section .text
.code16
.global _start_smp
.extern smp_done
_start_smp:
	# ensure that interrupts are disabled
	cli
	jmp .skip_data
.global smpboot_header
smpboot_header:
thread_stack:	.quad 0		# To be filled by the waking up code
gs_base:	.quad 0
ap_done:	.quad 0
.skip_data:
	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %gs
	mov %ax, %fs
	mov %ax, %es
	mov %ax, %ss
	mov %eax, %cr0
	mov $SMP_TRAMPOLINE_BASE + _gdtr_begin - _start_smp, %eax
	lgdt (%eax)
	mov %cr0, %eax
	or $1, %al
	mov %eax, %cr0
	jmp $0x08, $SMP_TRAMPOLINE_BASE + __long_mode_enter - _start_smp
.code32
__long_mode_enter:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %gs
	mov %ax, %fs
	mov %ax, %es
	mov %ax, %ss
	.extern pml4
	# Load the kernel's pml4
	mov $pml4, %eax
	mov %eax, %cr3
	# Enable PAE
	mov %cr4, %eax
	or $1 << 5, %eax
	mov %eax, %cr4
	/* Enable Long Mode in the MSR
	 * Use this to perform some initialization of features
	 * we might need
	*/

	mov $0xC0000080, %ecx
	rdmsr
	or $1, %eax # enable syscall/sysret
	or $1 << 8, %eax # enable long mode
	or $1 << 11, %eax # enable NX
	wrmsr

	# Enable Paging
	mov %cr0, %eax
	or $1 << 31, %eax
	mov %eax, %cr0
	mov $SMP_TRAMPOLINE_BASE + _gdtr2_begin - _start_smp, %eax
	lgdt (%eax)

	mov $stack - _start_smp, %esp
	push $0x08
	push $0x0 + _long_mode - _start_smp
	lret
_long_mode:
.code64
	.extern pml4
	mov $pml4, %rax
	mov  $.higher_half, %rbx
	jmp *%rbx
.higher_half:
	.extern gdtr
	mov $gdtr, %rbx
	lgdt (%rbx)
	mov %rax, %cr3
	mov $0x10, %rax
	mov %ax, %ds
	mov %ax, %gs
	mov %ax, %fs
	mov %ax, %es
	mov %ax, %ss

	# Load the shared IDT
	.extern idt
	lidt (idt)
	mov $(thread_stack - _start_smp), %rbx
	mov (%rbx), %rsp
	.global boot_pml4
	mov $boot_pml4, %rbx
	mov (%rbx), %rax
	mov %rax, %cr3

	mov %cr0, %rax
	and $0xFFFB, %ax
	or $0x2, %ax
	mov %rax, %cr0
	mov %cr4, %rax
	or $3 << 9, %ax
	or $1 << 18, %eax
	mov %rax, %cr4
	push $0x1F80
	ldmxcsr (%rsp)
	add $8, %rsp
	mov $(gs_base - _start_smp), %rdx
	mov $0xffff880000000000, %rax
	add %rax, %rdx
	mov (%rdx), %rdi
	# Say to the BSP that we finished
	mov $(ap_done - _start_smp), %rbx
	add %rax, %rbx
	lock incb (%rbx)
	
	call smpboot_main
halt:
	hlt
	jmp halt

.align 16
stack:
.skip 2048
.global _smp_func_end
_smp_func_end:
gdt:
	.quad	0x0000000000000000
	.quad	0x00CF9A000000FFFF
	.quad	0x00CF92000000FFFF
gdtl:
	.quad 	0x0000000000000000
	.quad	0x00A09A0000000000
	.quad	0x00A0920000000000
	.quad	0x00A0FA0000000000
	.quad	0x00A0F20000000000
tss_gdt:
	.word	0x67,0
	.byte	0, 0xE9, 0
	.byte	0
	.long	0

_gdtr_begin:
	.word	40
	.long	0x0 + gdt - _start_smp
_gdtr2_begin:
gdtr2:
	.word 40
	.long 0x0 + gdtl - _start_smp
.global _end_smp
_end_smp:
.section .text
.global __idle
__idle:
	cli
	hlt
	jmp __idle