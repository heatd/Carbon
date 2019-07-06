/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <carbon/compiler.h>
#include <stdio.h>
USES_FANCY_START
#include <immintrin.h>
#include <x86intrin.h>
USES_FANCY_END

#include <stdint.h>
#include <carbon/x86/avx.h>
#include <carbon/x86/cpu.h>
#include <carbon/x86/control_regs.h>

namespace Fpu
{

bool avx_supported = false;
USES_FANCY_START

void do_xsave(void *address, long xcr0)
{
	_xsave(address, xcr0);
}

void do_fxsave(void *address)
{
	_fxsave(address);
}

void do_xrstor(void *address, long xcr0)
{
	_xrstor(address, xcr0);
}

void do_fxrstor(void *address)
{
	_fxrstor(address);
}
USES_FANCY_END

void SaveFpu(unsigned char *address)
{
	if(avx_supported == true)
	{
		do_xsave(address, AVX_XCR0_FPU | AVX_XCR0_SSE | AVX_XCR0_AVX);
	}
	else
	{
		do_fxsave(address);
	}
}

void RestoreFpu(unsigned char *address)
{
	if(avx_supported == true)
	{
		do_xrstor(address, AVX_XCR0_FPU | AVX_XCR0_SSE | AVX_XCR0_AVX);
	}
	else
	{
		do_fxrstor(address);
	}
}

struct fpu_area
{
	uint16_t fcw;
	uint16_t fsw;
	uint8_t ftw;
	uint8_t res0;
	uint16_t fop;
	uint32_t fpu_ip;
	uint32_t fpu_cs;
	uint32_t fpu_dp;
	uint16_t ds;
	uint16_t res1;
	uint32_t mxcsr;
	uint32_t mxcsr_mask;
	uint8_t registers[0];
} __attribute__((packed));

void SetupFpuArea(unsigned char *address)
{
	struct fpu_area *area = (struct fpu_area*) address;
	area->mxcsr = 0x1F80;
}

static inline void xsetbv(unsigned long r, unsigned long xcr0)
{
	__asm__ __volatile__("xsetbv"::"c"(r), "a"(xcr0 & 0xffffffff), "d"(xcr0 >> 32));
}

static inline unsigned long xgetbv(unsigned long r)
{
	unsigned long ret = 0;
	__asm__ __volatile__("xgetbv":"=A"(ret):"c"(r));
	return ret;
}

void Init(void)
{
	if(x86::Cpu::HasCap(X86_FEATURE_AVX) && x86::Cpu::HasCap(X86_FEATURE_XSAVE))
	{
		x86::WriteCr4(x86::ReadCr4() | CR4_OSXSAVE);
		avx_supported = true;
		/* If it's supported, set the proper xcr0 bits */
		int64_t xcr0 = 0;

		xcr0 |= AVX_XCR0_AVX | AVX_XCR0_FPU | AVX_XCR0_SSE;

		xsetbv(0, xcr0);
	}
}

}