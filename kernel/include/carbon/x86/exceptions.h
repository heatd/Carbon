
#ifndef _CARBON_EXCEPTIONS_H
#define _CARBON_EXCEPTIONS_H

#include <stdint.h>
#include <carbon/x86/idt.h>

typedef struct
{
	uint64_t ds;
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsi, rdi, rdx, rcx, rbx, rax;
	uint64_t int_no;
	uint64_t err_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
} __attribute__((packed)) intctx_t;

#endif