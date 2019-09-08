/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_HASHTABLE_H
#define _CARBON_HASHTABLE_H

#include <carbon/list.h>

namespace cul
{

template <typename T, size_t number_entries,
	typename hash_type, hash_type (*hash_func)(T& data)>
class hashtable
{
private:
	LinkedList<T> buckets[number_entries];
public:
	hashtable() : buckets() {}

	void empty_bucket(size_t i)
	{
		auto& bucket = buckets[i];
		
		for(auto it = bucket.begin(); it != bucket.end();)
		{
			auto elem = *it;
			auto to_delete = it++;
			bucket.Remove(elem, to_delete);
			delete elem;
		}
	}

	void empty()
	{
		for(size_t i = 0; i < number_entries; i++)
		{
			if(buckets[i].IsEmpty())
				continue;
			empty_bucket(i);
		}
	}

	~hashtable()
	{
		empty();	
	}

	bool add_element(const T& elem)
	{
		auto hash = hash_func(const_cast<T&>(elem));

		auto index = hash % number_entries;
		return buckets[index].Add(elem);
	}

	LinkedListIterator<T> get_hash_list_begin(hash_type hash)
	{
		auto index = hash % number_entries;
		return buckets[index].begin();
	}

	LinkedListIterator<T> get_hash_list_end(hash_type hash)
	{
		auto index = hash % number_entries;
		return buckets[index].end();
	}

	bool remove_element(T& elem, hash_type hash, LinkedListIterator<T> it)
	{
		auto index = hash % number_entries;
		
		return buckets[index].Remove(elem, it);
	}

	bool remove_element(T& elem)
	{
		auto hash = hash_func(elem);
		auto index = hash % number_entries;

		return remove_element(elem, hash, buckets[index].begin());
	}
};

};

#endif