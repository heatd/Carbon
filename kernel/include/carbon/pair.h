/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PAIR_H
#define _CARBON_PAIR_H

namespace cul
{

template <typename type1, typename type2>
class pair
{
public:
	using first_type = type1;
	using second_type = type2;
	type1 first_member;
	type2 second_member;

	constexpr pair(const type1& first, const type2& second) :
		first_member(first), second_member(second) {}
	constexpr pair(type1&& first, type2&& second) : first_member(first), second_member(second)
	{}

	constexpr bool operator==(const pair<type1, type2>& rhs)
	{
		if(rhs.first_member == first_member &&
		   rhs.second_member == second_member)
		   	return true;
		return false;
	}

	constexpr bool operator!=(const pair<type1, type2>& rhs)
	{
		if(rhs.first_member != first_member ||
		   rhs.second_member != second_member)
		   	return true;
		return false;
	}
};

};

#endif