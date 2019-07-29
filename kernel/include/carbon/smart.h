/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _SMART_KERNEL_H
#define _SMART_KERNEL_H

#include <assert.h>

template <typename T>
class refcount
{
private:
	long ref;
	T *data;
public:
	refcount() : ref(1), data(nullptr){}
	refcount(T *d): ref(1), data(d){}
	~refcount()
	{
		if(data)
			delete data;
	}

	long get_ref(void)
	{
		return ref;
	}

	T *get_data(void)
	{
		return data;
	}

	void release(void)
	{
		/* 1 - 1 = 0! */
		if(__sync_fetch_and_sub(&ref, 1) == 1)
		{
			delete data;
			data = nullptr;
			/* Commit sudoku */
			delete this;
		}
	}
	
	void refer(void)
	{
		__sync_fetch_and_add(&ref, 1);
	}
};

template <typename T>
class smart_ptr
{
private:
	refcount<T> *ref;
public:
	smart_ptr() : ref(nullptr) {}
	smart_ptr(T *data)
	{
		ref = new refcount<T>(data);
	}

	smart_ptr(const smart_ptr<T>& ptr)
	{
		ptr.ref->refer();
		ref = ptr.ref;
	}

	smart_ptr(smart_ptr<T>&& ptr) : ref(ptr.ref)
	{
		ptr.ref = nullptr;
	}

	smart_ptr& operator=(smart_ptr<T>&& ptr)
	{
		if(ref)
		{
			ref->release();
			ref = nullptr;
		}

		ref = ptr.ref;
		ptr.ref = nullptr;

		return *this;
	}

	smart_ptr& operator=(const smart_ptr& p)
	{
		if(ref == p.ref)
			goto ret;
		if(ref)
		{
			ref->release();
			ref = nullptr;
		}

		if(p.ref)
		{
			p.ref->refer();
			ref = p.ref;
		}
	ret:
		return *this;
	}

	bool operator==(const smart_ptr& p)
	{
		return (p.ref == ref);
	}

	bool operator==(const T *ptr)
	{
		if(!ref && !ptr)
			return true;
		else if(!ref)
			return false;
		
		return (ref->get_data() == ptr);
	}

	bool operator!=(const smart_ptr& p)
	{
		return !operator==(p);
	}

	bool operator!=(const T *ptr)
	{
		return !operator==(ptr);
	}
	
	~smart_ptr(void)
	{
		/* Order this in order to be thread-safe */
		auto r = ref;
		ref = nullptr;

		if(r) r->release();
	}

	T* get_data()
	{
		if(!ref)
			return nullptr;
		return ref->get_data();
	}

	T& operator*()
	{
		return *get_data();
	}
	
	T* operator->()
	{
		return get_data();
	}

	bool operator!()
	{
		return get_data() == nullptr;
	}
	
	operator bool()
	{
		return get_data() != nullptr;
	}

	refcount<T> *__get_refc() const
	{
		return ref;
	}

	void __set_refc(refcount<T> *r)
	{
		if(ref)
			ref->release();
		r->refer();
		ref = r;
	}
};

namespace smartptr
{

template <typename T, class ... Args>
smart_ptr<T> make(Args && ... args)
{
	T *data = new T(args...);
	if(!data)
		return nullptr;

	smart_ptr<T> p(data);
	return p;
}

template <typename T, typename U>
smart_ptr<T> cast(const smart_ptr<U>& s)
{
	auto ref = s.__get_refc();
	smart_ptr<T> p{};
	p.__set_refc((refcount<T> *) ref);
	return p;
}

};
#endif
