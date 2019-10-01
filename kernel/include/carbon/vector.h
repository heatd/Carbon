/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_VECTOR_H
#define _CARBON_VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <carbon/compiler.h>
#include <carbon/array_iterator.h>
#include <carbon/panic.h>
#include <carbon/new.h>
#include <stdio.h>

template <typename T>
class vector
{
private:
	T *data;
	unsigned long buffer_size;
	unsigned long nr_elems;
	unsigned long log;

	void setup_expansion(size_t new_nr_elems)
	{
		for(size_t i = nr_elems; i < new_nr_elems; i++)
		{
			new (data + i) T{};
		}
	}

	bool expand_vec()
	{
		size_t new_buf_elems = 1 << log;
		size_t new_buffer_size = new_buf_elems * sizeof(T);
		T *new_data = reinterpret_cast<T*>(realloc((void *) data,
						   new_buffer_size));
		if(!new_data)
			return false;
		log++;
		data = new_data;
		setup_expansion(new_buf_elems);
		buffer_size = new_buf_elems;
		return true;
	}
public:
	constexpr vector() : data{nullptr}, buffer_size{0}, nr_elems{0}, log{0}
	{

	}

	bool alloc_buf(size_t size)
	{
		auto old_log = log;
		auto _log = ilog2(size);
		if(size & ((1 << _log) - 1))
			_log++;
		
		assert(nr_elems <= size);

		log = _log;
		if(!expand_vec())
		{
			/* Revert */
			log = old_log;
			return false;
		}

		return true;
	}

	bool push_back(const T& obj)
	{
		if(nr_elems >= buffer_size)
		{
			if(!expand_vec())
				return false;
		}

		data[nr_elems++] = obj;

		return true;
	}

	bool push_back(T&& obj)
	{
		if(nr_elems >= buffer_size)
		{
			if(!expand_vec())
				return false;
		}

		data[nr_elems++] = obj;

		return true;
	}

	void clear()
	{
		if(!data)
			return;
		for(unsigned long i = 0; i < nr_elems; i++)
		{
			T& ref = data[i];
			ref.~T();
		}

		free(data);
		data = nullptr;
		buffer_size = 0;
		nr_elems = 0;
	}

	~vector()
	{
		if(data)
		{
			clear();
		}
	}

	T& operator[](unsigned long idx)
	{
		if(idx >= nr_elems)
		{
			panic_bounds_check(this, true, idx);
		}

		return data[idx];
	}

	size_t size() const
	{
		return nr_elems;
	}

	size_t buf_size() const
	{
		return buffer_size;
	}

	void set_nr_elems(size_t nr)
	{
		nr_elems = nr;
	}

	array_iterator<T> begin()
	{
		return array_iterator<T>{data};
	}

	array_iterator<T> end()
	{
		return array_iterator<T>{&data[nr_elems]};
	}

	const_array_iterator<T> cbegin()
	{
		return const_array_iterator<T>{data};
	}

	const_array_iterator<T> cend()
	{
		return const_array_iterator<T>{&data[nr_elems]};
	}

	T *get_buf()
	{
		return data;
	}

	const T *get_buf() const
	{
		return data;
	}
};

#endif