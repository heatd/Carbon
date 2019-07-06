/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_CLOCKSOURCE_H
#define _CARBON_CLOCKSOURCE_H

#include <stdint.h>

#define NS_PER_SEC 	1000000000UL

class ClockSource
{
private:
	const char *name;
	unsigned int rate;
	unsigned int rating;
public:
	using ClockTicks = uint64_t;
	using ClockNs = uint64_t;

	ClockSource(const char *name, unsigned int rate, unsigned int rating)
		: name(name), rate(rate), rating(rating) {}
	virtual ~ClockSource() {}

	virtual ClockTicks GetTicks() = 0;
	virtual ClockNs GetElapsed(ClockTicks first, ClockTicks last) = 0;
};

namespace Time
{

unsigned long GetTicks();

};

#endif