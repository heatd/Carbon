/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <carbon/x86/msr.h>
#include <carbon/x86/segments.h>

#define ENTROPY_BITS	29
#define NR_ENTROPY	32
.section .bss
.align 16
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

.section .data

.align 16
gdt_begin: 
        .quad 0x0000000000000000   # 0x0  - NULL segment
        .quad 0x00A09A0000000000   # 0x8  - KERNEL CS
        .quad 0x00A0920000000000   # 0x10 - KERNEL DS
        .quad 0x00CFFA000000FFFF   # 0x18 - 32-bit user CS
        .quad 0x00CFF2000000FFFF   # 0x20 - 32-bit user DS
        .quad 0x00A0FA0000000000   # 0x28 - USER CS
        .quad 0x00A0F20000000000   # 0x30 - USER DS
.global tss_gdt                    # 0x38 - TSS
tss_gdt: 
        .word 0x67,0
        .byte 0,0xE9,0
        .byte 0
        .long 0
.align 16
.global gdtr
gdtr: 
        .word 72 - 1
        .quad gdt_begin

.section .boot
.global calculate_base_address
calculate_base_address:
	/* Get the first 4 bytes of entropy and use them to calculate the base address */
	xor %rax, %rax
	mov (%rdi), %rax

#ifdef CONFIG_RELOCATABLE
	and $((1 << ENTROPY_BITS) - 1), %rax
	or $0xffffffff80000000, %rax
	and $-0x1000000, %rax
#else
	mov $0xffffffff80000000, %rax
#endif

	movq %rax, base_address
	ret
.global _start
_start:
	cli
	mov $(EFER_SCE | EFER_NXE | EFER_LME), %rax
	xor %rdx, %rdx
	mov $MSR_EFER, %rcx
	wrmsr

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
	
	# Load a zero-sized IDT
	lidt idt
	# And load the proper gdt

	mov $gdtr, %rax
	lgdt (%rax)

	# Load segments and lret (retf) to flush the code segment

	mov $KERNEL_DS, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %es

	pushq $KERNEL_CS
	mov $.cs_flush, %rax
	add (base_address), %rax
	pushq %rax

	lretq
.cs_flush:
	mov $efi_entry, %rax
	call *%rax
	cli
.halt:
	hlt
	jmp .halt
