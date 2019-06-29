/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_INTERRUPT_H
#define _CARBON_INTERRUPT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __x86_64__
#define ARCH_MAX_VECTORS	256
#endif

namespace Interrupt
{

using InterruptVector = uint32_t;

constexpr InterruptVector MaxVector = ARCH_MAX_VECTORS;
constexpr InterruptVector InvalidVector = (InterruptVector) -1;

void ReserveInterrupts(InterruptVector base, size_t number);
InterruptVector AllocateInterrupts(size_t number);
void FreeInterrupts(InterruptVector vector, size_t number);

};

#endif