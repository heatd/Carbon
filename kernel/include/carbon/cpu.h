/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_CPU_H
#define _CARBON_CPU_H

#ifdef __x86_64__

#include <carbon/x86/apic.h>

#endif

#include <carbon/lock.h>

namespace Scheduler
{

struct thread;
};

using Scheduler::thread;

#define NR_PRIO		40

#endif