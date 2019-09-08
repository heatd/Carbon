/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_ARRAY_H
#define _CARBON_ARRAY_H

#include <stdio.h>
#include <carbon/panic.h>
#include <carbon/array_iterator.h>

/* Implements an std::array-like construct */

template <typename T, unsigned long nr_elem>
class array
{
private:

public:
	T data[nr_elem];

	constexpr T& operator[](unsigned long index)
	{
		if(index >= nr_elem)
			panic_bounds_check(this, false, index);
		return data[index];
	}

	constexpr const T& operator[](unsigned long index) const
	{
		if(index >= nr_elem)
			panic_bounds_check(this, false, index);
		return (const T&) data[index];
	}

	unsigned long size() const
	{
		return nr_elem;
	}

	array_iterator<T> begin()
	{
		return array_iterator<T>(data);
	}

	array_iterator<T> end()
	{
		return array_iterator<T>(data + nr_elem);
	}

	const_array_iterator<T> cbegin()
	{
		return const_array_iterator<T>(data);
	}

	const_array_iterator<T> cend()
	{
		return const_array_iterator<T>(data + nr_elem);
	}
};

template <typename T, unsigned long nr_elem>
constexpr bool operator==(const array<T, nr_elem>& lhs, const array<T, nr_elem>& rhs)
{
	for(auto i = 0UL; i < nr_elem; i++)
	{
		if(lhs.data[i] != rhs.data[i])
			return false;
	}

	return true;
}

template <typename T, unsigned long nr_elem>
constexpr bool operator!=(const array<T, nr_elem>& lhs, const array<T, nr_elem>& rhs)
{
	return !(lhs == rhs);
}

#endif