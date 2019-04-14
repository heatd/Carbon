/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <carbon/x86/exceptions.h>
#include <carbon/panic.h>

void div0_exception(intctx_t *ctx)
{
	panic("Division by zero exception!");
}

void debug_trap(intctx_t *ctx)
{
	panic("Debug trap!");
}

void nmi_exception(intctx_t *ctx)
{
	panic("Non-maskable interrupt detected!");
}

void breakpoint_exception(intctx_t *ctx)
{
	panic("Breakpoint exception");
}

void overflow_trap(intctx_t *ctx)
{
	panic("Overflow trap");
}

void boundrange_exception(intctx_t *ctx)
{
	panic("Boundrange exception");
}

void invalid_opcode_exception(intctx_t *ctx)
{
	panic("Invalid opcode exception");
}

void device_not_avail_excp(intctx_t *ctx)
{
	panic("Device not available exception");
}

void __double_fault(intctx_t *ctx)
{
	__asm__ __volatile__("hlt");
}

void exception_panic(intctx_t *ctx)
{
	panic("Unknown exception");
}

void invalid_tss_exception(intctx_t *ctx)
{
	panic("Invalid tss exception");
}

void segment_not_present_excp(intctx_t *ctx)
{
	panic("Segment not present exception");
}

void stack_segment_fault(intctx_t *ctx)
{
	panic("Stack segment fault");
}

void general_protection_fault(intctx_t *ctx)
{
	panic("General protection fault");
}

void page_fault_handler(intctx_t *ctx)
{
	uint64_t error_code = ctx->err_code;
	bool write = error_code & 0x2;
	bool read = write ? 0 : 1;
	bool exec = error_code & 0x10;
	bool user = error_code & 0x4;

	printf("Page fault info: ");

	if(write)
		printf("W ");
	if(read)
		printf("R ");
	if(exec)
		printf("X ");
	if(user)
		printf("U ");
	printf(" \n");

	printf("Page fault occurred at %lx\n", ctx->rip);
	panic("Page fault");
}

void x87_fpu_exception(intctx_t *ctx)
{
	panic("x87 fpu exception");
}

void alignment_check_excp(intctx_t *ctx)
{
	panic("Alignment check");
}

void machine_check(intctx_t *ctx)
{
	panic("Machine check");
}

void simd_fpu_exception(intctx_t *ctx)
{
	panic("SIMD fpu exception");
}

void virtualization_exception(intctx_t *ctx)
{
	panic("Virtualization exception");
}

void security_exception(intctx_t *ctx)
{
	panic("Security exception");
}

void (* const int_handlers[])(intctx_t *ctx) = 
{
	div0_exception,
	debug_trap,
	nmi_exception,
	breakpoint_exception,
	overflow_trap,
	boundrange_exception,
	invalid_opcode_exception,
	device_not_avail_excp,
	__double_fault,
	exception_panic,
	invalid_tss_exception,
	segment_not_present_excp,
	stack_segment_fault,
	general_protection_fault,
	page_fault_handler,
	exception_panic,
	x87_fpu_exception,
	alignment_check_excp,
	machine_check,
	simd_fpu_exception,
	virtualization_exception,
	exception_panic,
	exception_panic,
	exception_panic,
	exception_panic,
	exception_panic,
	exception_panic,
	exception_panic,
	exception_panic,
	exception_panic,
	security_exception,
	exception_panic
};

extern "C" void x86_exception_gate(intctx_t *ctx)
{
	int_handlers[ctx->int_no](ctx);
}