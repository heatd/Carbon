/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

.section .text
.align 32
.global __raw_asm_get_per_cpu
__raw_asm_get_per_cpu:
	# rdi - offset
	# rsi - size

	cmp $8, %rsi
	jne .other_cases
	movq %gs:(%rdi), %rax
	ret
.other_cases:
	
	# we need to zero out %rax
	xor %rax, %rax
	cmp $4, %rsi
	je .get_4bytes
	cmp $2, %rsi
	je .get_2bytes
	
	# 1 byte
	movb %gs:(%rdi), %al
	ret
.get_2bytes:
	# 2 bytes
	movw %gs:(%rdi), %ax
	ret
.get_4bytes:
	# 4 bytes
	movl %gs:(%rdi), %eax
	ret

.align 32
.global __raw_asm_write_per_cpu
__raw_asm_write_per_cpu:
	# rdi - off
	# rsi - val
	# rdx - size
	cmp $8, %rdx
	jne __raw_asm_write_per_cpu.other_cases
	movq %rsi, %gs:(%rdi)
	ret
__raw_asm_write_per_cpu.other_cases:
	cmp $4, %rdx
	je .write_4bytes
	cmp $2, %rdx
	je .write_2bytes

	# 1 byte
	movb %sil, %gs:(%rdi)
	ret
.write_2bytes:
	# 2 bytes
	movw %si, %gs:(%rdi)
	ret
.write_4bytes:
	# 4 bytes
	movl %esi, %gs:(%rdi)
	ret

.align 32
.global __raw_asm_add_per_cpu
__raw_asm_add_per_cpu:
	# rdi - off
	# rsi - val
	# rdx - size
	cmp $8, %rdx
	jne __raw_asm_add_per_cpu.other_cases
	addq %rsi, %gs:(%rdi)
	ret
__raw_asm_add_per_cpu.other_cases:
	cmp $4, %rdx
	je .add_4bytes
	cmp $2, %rdx
	je .add_2bytes

	# 1 byte
	addb %sil, %gs:(%rdi)
	ret
.add_2bytes:
	# 2 bytes
	addw %si, %gs:(%rdi)
	ret
.add_4bytes:
	# 4 bytes
	addl %esi, %gs:(%rdi)
	ret