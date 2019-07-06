/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>
#include <carbon/percpu.h>
#include <carbon/smp.h>
#include <carbon/scheduler.h>
#include <carbon/memory.h>
#include <carbon/vm.h>

#include <carbon/x86/msr.h>
#include <carbon/x86/apic.h>
#include <carbon/x86/gdt.h>
#include <carbon/fpu.h>

void idt_load();

extern "C"
void smpboot_main(unsigned long gs_base)
{
	/* Write the base to the GS_BASE */
	wrmsr(GS_BASE_MSR, gs_base);

	idt_load();

	Gdt::InitPercpu();

	Fpu::Init();

	x86::Apic::SetupLapic();

	while(true)
		__asm __volatile__("hlt");
}