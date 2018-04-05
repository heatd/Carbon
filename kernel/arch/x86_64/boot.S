/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#define NR_ENTROPY	32
.section .bss
.align 4
stack_bottom:
	.skip 8192
stack_top:
.section .boot
.align 4096
.global pml4
pml4:
	.skip 4096
.global pdpt
pdpt:
	.skip 4096
.global pdpt2
pdpt2:
	.skip 4096
.global pdlower
pdlower:
	.skip 4096
.global pdhigher
pdhigher:
	.skip 4096
.global base_address
base_address: .quad 0xffffffff80000000

idt:
	.quad 0
	.word 0
.global calculate_base_address
calculate_base_address:
	/* Get the first 4 bytes of entropy and use them to calculate the base address */
	xor %rax, %rax
	mov (%rdi), %rax
	mov $0xffffffff80000000, %rax
	and $-0x1000000, %rax
	movq %rax, base_address
	ret
.global _start
_start:
	cli
	call calculate_base_address
	mov %rax, %rsi
	mov $pml4, %rax
	push %rdi
	push %rax
	call relocate_kernel
	pop %rax
	pop %rdi
	mov %rax, %cr3
	mov $.higher, %rax
	jmp *%rax
.higher:
	mov $stack_top, %rsp
	lidt idt
	call efi_entry
	cli
.halt:
	hlt
	jmp .halt
