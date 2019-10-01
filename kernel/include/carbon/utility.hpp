/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_UTILTY_H
#define _CARBON_UTILTY_H

namespace cul
{

template <typename T>
struct remove_reference {typedef T type; };

template <typename T>
struct remove_reference<T&> { typedef T type; };

template <typename T>
struct remove_reference<T&&> { typedef T type; };

template <typename T>
typename remove_reference<T>::type move(T&& t)
{
	return static_cast<typename remove_reference<T>::type&& >(t);
}

template <typename T>
void swap(T& a, T& b)
{
	T temp(move(a));
	a = move(b);
	b = move(temp);
}

}

#endif