#ifndef _CARBON_X86_MSR_H
#define _CARBON_X86_MSR_H

#define MSR_EFER        	0xC0000080
#define FS_BASE_MSR 		0xC0000100
#define GS_BASE_MSR 		0xC0000101
#define KERNEL_GS_BASE 		0xC0000102
#define IA32_MSR_STAR 		0xC0000081
#define IA32_MSR_LSTAR 		0xC0000082
#define IA32_MSR_CSTAR 		0xC0000083
#define IA32_MSR_SFMASK 	0xC0000084
#define IA32_MSR_MC0_CTL 	0x00000400
#define IA32_MSR_PAT		0x00000277


#define EFER_SCE			(1 << 0)
#define EFER_LME			(1 << 8)
#define EFER_LMA			(1 << 10)
#define EFER_NXE			(1 << 11)
#define EFER_SVME			(1 << 12)
#define EFER_LMSLE			(1 << 13)
#define EFER_FFXSR			(1 << 14)
#define EFER_TCE			(1 << 15)

#ifndef __ASSEMBLER__

#include <stdint.h>

inline void wrmsr(uint32_t msr, uint64_t val)
{
	uint32_t lo = (uint32_t) val;
	uint32_t hi = val >> 32;
	__asm__ __volatile__("wrmsr"::"a"(lo), "d"(hi), "c"(msr));
}

inline uint64_t rdmsr(uint32_t msr)
{
	uint32_t lo, hi;
	__asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));

	return (uint64_t) lo | ((uint64_t) hi << 32);
}

inline uint64_t rdtsc(void)
{
    	uint64_t ret = 0;
    	__asm__ __volatile__ ("rdtsc" : "=A"(ret));
    	return ret;
}

#endif


#endif