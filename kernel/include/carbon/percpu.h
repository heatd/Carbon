/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_PERCPU_H
#define _CARBON_PERCPU_H

#include <carbon/compiler.h>

unsigned int get_cpu_nr();

#define PER_CPU_VAR(var) __attribute__((section(".percpu"), used))	var

#if 1

#define get_per_cpu(var) 			\
({						\
	unsigned long val;			\
	__asm__ __volatile__("movq %%gs:" 	\
	"%p1, %0" : "=r"(val) : "i"(&var)); 	\
	(decltype(var)) val;			\
})

#define get_per_cpu_no_cast(var) 		\
({						\
	unsigned long val;			\
	__asm__ __volatile__("movq %%gs:" 	\
	"%p1, %0" : "=r"(val) : "i"(&var)); 	\
	val;					\
})

#define write_per_cpu_1(var, val) 		\
({						\
	__asm__ __volatile__("movb %0, %%gs:"   \
	"%p1":: "r"(val), "i"(&var)); 		\
})

#define write_per_cpu_2(var, val) 		\
({						\
	__asm__ __volatile__("movw %0, %%gs:"   \
	"%p1":: "r"(val), "i"(&var)); 		\
})

#define write_per_cpu_4(var, val) 		\
({						\
	__asm__ __volatile__("movl %0, %%gs:"   \
	"%p1":: "r"(val), "i"(&var)); 		\
})

#define write_per_cpu_8(var, val) 		\
({						\
	__asm__ __volatile__("movq %0, %%gs:"   \
	"%p1":: "r"((unsigned long) val), "i"(&var)); 		\
})

#define write_per_cpu(var, val)				\
({							\
	switch(sizeof(var))				\
	{						\
		case 1:					\
			write_per_cpu_1(var, val);	\
			break;				\
		case 2:					\
			write_per_cpu_2(var, val);	\
			break;				\
		case 4:					\
			write_per_cpu_4(var, val);	\
			break;				\
		case 8:					\
			write_per_cpu_8(var, val);	\
			break;				\
	}						\
})

#define add_per_cpu_1(var, val) 		\
({						\
	__asm__ __volatile__("addb %0, %%gs:"   \
	"%p1":: "r"(val), "i"(&var)); 		\
})

#define add_per_cpu_2(var, val) 		\
({						\
	__asm__ __volatile__("addw %0, %%gs:"   \
	"%p1":: "r"(val), "i"(&var)); 		\
})

#define add_per_cpu_4(var, val) 		\
({						\
	__asm__ __volatile__("addl %0, %%gs:"   \
	"%p1":: "r"(val), "i"(&var)); 		\
})

#define add_per_cpu_8(var, val) 		\
({						\
	__asm__ __volatile__("addq %0, %%gs:"   \
	"%p1":: "r"((unsigned long) val), "i"(&var)); 		\
})

#define add_per_cpu(var, val)				\
({							\
	switch(sizeof(var))				\
	{						\
		case 1:					\
			add_per_cpu_1(var, val);	\
			break;				\
		case 2:					\
			add_per_cpu_2(var, val);	\
			break;				\
		case 4:					\
			add_per_cpu_4(var, val);	\
			break;				\
		case 8:					\
			add_per_cpu_8(var, val);	\
			break;				\
	}						\
})

#else

extern "C"
unsigned long __raw_asm_get_per_cpu(size_t off, size_t size);
extern "C"
void __raw_asm_write_per_cpu(size_t off, unsigned long val, size_t size);
extern "C"
void __raw_asm_add_per_cpu(size_t off, unsigned long val, size_t size);

#define get_per_cpu_no_cast(var)	__raw_asm_get_per_cpu((size_t) &var, sizeof(var))
#define get_per_cpu(var)		((decltype(var)) __raw_asm_get_per_cpu((size_t) &var, sizeof(var)))
#define write_per_cpu(var, val)			__raw_asm_write_per_cpu((size_t) &var, (unsigned long) val, sizeof(var))
#define add_per_cpu(var, val)			__raw_asm_add_per_cpu((size_t) &var, (unsigned long) val, sizeof(var))

#endif
#ifdef __cplusplus
namespace Percpu
{

extern unsigned long *percpu_bases;
void Init();
unsigned long InitForCpu(unsigned int cpu);

};

#define other_cpu_get_ptr(var, cpu)		((decltype(var) *) (Percpu::percpu_bases[cpu] + (unsigned long) &var))
#define other_cpu_get(var, cpu)			*other_cpu_get_ptr(var, cpu)
#define other_cpu_write(var, val, cpu)		*other_cpu_get_ptr(var, cpu) = val
#define other_cpu_add(var, val, cpu)		*other_cpu_get_ptr(var, cpu) += val

#define get_per_cpu_ptr_any(var, cpu)	(cpu == get_cpu_nr() ? get_per_cpu_ptr(var) : other_cpu_get_ptr(var, cpu))
#define get_per_cpu_any(var, cpu)	(cpu == get_cpu_nr() ? get_per_cpu(var) : other_cpu_get(var, cpu))

#define write_per_cpu_any(var, val, cpu)			\
do {								\
if(cpu == get_cpu_nr())						\
	write_per_cpu(var, val); 				\
else other_cpu_write(var, val, cpu);				\
} while(0)

#define add_per_cpu_any(var, val, cpu)		\
do {						\
if(cpu == get_cpu_nr())				\
	add_per_cpu(var, val); 			\
else other_cpu_add(var, val, cpu);		\
} while(0)

extern "C"
#endif
unsigned long __cpu_base;

#define get_per_cpu_ptr_no_cast(var)							\
({											\
	unsigned long ___cpu_base = get_per_cpu_no_cast(__cpu_base);			\
	((unsigned long) &var + ___cpu_base);						\
})

#ifdef __cplusplus
#define get_per_cpu_ptr(var)								\
({											\
	(decltype(var) *) get_per_cpu_ptr_no_cast(var);					\
})
#endif

#endif