/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <carbon/percpu.h>
#include <carbon/page.h>
#include <carbon/memory.h>
#include <carbon/bootprotocol.h>
#include <carbon/x86/serial.h>
#include <carbon/framebuffer.h>
#include <carbon/page.h>
#include <carbon/x86/idt.h>
#include <carbon/vm.h>
#include <carbon/x86/cpu.h>
#include <carbon/x86/apic.h>
#include <carbon/acpi.h>
#include <carbon/list.h>
#include <carbon/scheduler.h>
#include <carbon/cpu.h>
#include <carbon/smp.h>

#include <carbon/fpu.h>
#include <carbon/x86/gdt.h>

void heap_set_start(uintptr_t heap_start);
void paging_protect_kernel(void);
extern "C" void _init();

void x86_init(struct boot_info *info)
{
	paging_protect_kernel();

	x86_init_exceptions();

	uintptr_t heap_start = 0xffffa00000000000;
	heap_set_start(heap_start);

	vm_init();

#if 0
	asan_init();
#endif

	/* Invoke global constructors */
	_init();

	Percpu::Init();

	Gdt::InitPercpu();

	Fpu::Init();

	Scheduler::Initialize();

	Acpi::Init();

	x86::Apic::Init();

	Smp::BootCpus();
}