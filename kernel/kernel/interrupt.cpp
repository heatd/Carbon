/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <limits.h>
#include <stdio.h>

#include <carbon/interrupt.h>
#include <carbon/lock.h>

namespace Interrupt
{

constexpr unsigned long vectors_per_entry = CHAR_BIT * sizeof(unsigned long);
constexpr unsigned long interrupt_bitmap_entries = MaxVector / vectors_per_entry;
static Spinlock bitmap_lock;
static unsigned long interrupt_bitmap[interrupt_bitmap_entries] = {};

inline bool IsFree(InterruptVector vector)
{
	unsigned long arr_index = vector / vectors_per_entry;
	unsigned long bit_index = vector % vectors_per_entry;

	return !(interrupt_bitmap[arr_index] & (1UL << bit_index));
}

inline void ReserveInterrupt(InterruptVector vector)
{
	unsigned long arr_index = vector / vectors_per_entry;
	unsigned long bit_index = vector % vectors_per_entry;

	interrupt_bitmap[arr_index] |= (1UL << bit_index);
}

void ReserveInterruptsUnlocked(InterruptVector base, size_t number)
{
	for(size_t i = 0; i < number; i++)
	{
		ReserveInterrupt(base++);
	}
}

void ReserveInterrupts(InterruptVector base, size_t number)
{
	ScopedSpinlock l(&bitmap_lock);
	ReserveInterruptsUnlocked(base, number);
}

InterruptVector AllocateInterrupts(size_t number)
{
	ScopedSpinlock l(&bitmap_lock);

	size_t to_alloc = number;
	InterruptVector base = 0;

	for(InterruptVector i = 0; i < MaxVector; i++)
	{
		if(IsFree(i))
		{
			if(to_alloc == number)
				base = i;

			to_alloc--;
		}
		else
		{
			to_alloc = number;
		}

		if(to_alloc == 0)
		{
			ReserveInterruptsUnlocked(base, number);
			return base;
		}
	}

	return InvalidVector;
}

inline void FreeInterrupt(InterruptVector vector)
{
	unsigned long arr_index = vector / vectors_per_entry;
	unsigned long bit_index = vector % vectors_per_entry;

	interrupt_bitmap[arr_index] &= ~(1UL << bit_index);
}

void FreeInterrupts(InterruptVector vector, size_t number)
{
	ScopedSpinlock l(&bitmap_lock);

	for(size_t i = 0; i < number; i++)
		FreeInterrupt(vector + i);
}

}