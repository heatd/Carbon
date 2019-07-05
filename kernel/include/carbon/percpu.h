/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_PERCPU_H
#define _CARBON_PERCPU_H

#define PER_CPU_VAR(var) __attribute__((section(".percpu"), used))	var

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)

#define get_per_cpu(var) 			\
({						\
	unsigned long val;			\
	__asm__ __volatile__("movq %%gs:" 	\
	"%p1, %0" : "=r"(val) : "i"(&var)); 	\
	(decltype(var)) val;			\
})

#define get_per_cpu_no_cast(var) 			\
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

namespace Percpu
{

extern unsigned long __cpu_base;
void Init();

};

#define get_per_cpu_ptr_no_cast(var)								\
({											\
	unsigned long ___cpu_base = get_per_cpu(Percpu::__cpu_base);			\
	((unsigned long) &var + ___cpu_base);				\
})

#define get_per_cpu_ptr(var)								\
({											\
	(decltype(var) *) get_per_cpu_ptr_no_cast(var);						\
})

#endif