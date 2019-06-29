/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_REGISTERS_H
#define _CARBON_REGISTERS_H

struct registers
{
	unsigned long ds;
	unsigned long r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsi, rdi, rdx, rcx, rbx, rax;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

#endif