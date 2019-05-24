#ifndef _CARBON_X86_MSR_H
#define _CARBON_X86_MSR_H

#define MSR_EFER        0xC0000080


#define EFER_SCE			(1 << 0)
#define EFER_LME			(1 << 8)
#define EFER_LMA			(1 << 10)
#define EFER_NXE			(1 << 11)
#define EFER_SVME			(1 << 12)
#define EFER_LMSLE			(1 << 13)
#define EFER_FFXSR			(1 << 14)
#define EFER_TCE			(1 << 15)
#endif