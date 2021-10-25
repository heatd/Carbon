/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_ALGORITHM_H
#define _CARBON_ALGORITHM_H

namespace cul
{

template <typename T>
constexpr auto& clamp(T& t1, T& min, T& max)
{
	return t1 < min ? min : (t1 > max ? max : t1);
}

template <typename T>
constexpr auto&& clamp(T&& t1, T&& min, T&& max)
{
	return t1 < min ? min : (t1 > max ? max : t1);
}

};


#endif