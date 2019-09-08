/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _SMART_KERNEL_H
#define _SMART_KERNEL_H

#include <assert.h>

#include <carbon/remove_extent.h>

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
class shared_ptr
{
private:
	refcount<T> *ref;
	using element_type = remove_extent_t<T>;
public:
	shared_ptr() : ref(nullptr) {}
	shared_ptr(T *data)
	{
		ref = new refcount<T>(data);
	}

	shared_ptr(const shared_ptr<T>& ptr)
	{
		ptr.ref->refer();
		ref = ptr.ref;
	}

	shared_ptr(shared_ptr<T>&& ptr) : ref(ptr.ref)
	{
		ptr.ref = nullptr;
	}

	shared_ptr& operator=(shared_ptr<T>&& ptr)
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

	shared_ptr& operator=(const shared_ptr& p)
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

	bool operator==(const shared_ptr& p)
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

	bool operator!=(const shared_ptr& p)
	{
		return !operator==(p);
	}

	bool operator!=(const T *ptr)
	{
		return !operator==(ptr);
	}

	element_type& operator[](size_t index)
	{
		return ref->data[index];
	}

	~shared_ptr(void)
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

template <typename T>
class unique_ptr
{
private:
	T *p;
	using element_type = remove_extent_t<T>;
public:
	unique_ptr() : p(nullptr) {}
	unique_ptr(T *data) : p(data)
	{
	}

	unique_ptr(const unique_ptr<T>& ptr) = delete;

	unique_ptr(unique_ptr<T>&& ptr) : p(ptr.p)
	{
		ptr.p = nullptr;
	}

	unique_ptr& operator=(unique_ptr<T>&& ptr)
	{
		if(p)
		{
			delete p;
		}

		this->p = ptr.p;

		return *this;
	}

	unique_ptr& operator=(const unique_ptr& p)
	{
		if(this->p == p.p)
	ret:
		return *this;
	}

	bool operator==(const unique_ptr& p)
	{
		return (p == p.p);
	}

	bool operator==(const T *ptr)
	{	
		return (p == ptr);
	}

	bool operator!=(const unique_ptr& p)
	{
		return !operator==(p);
	}

	bool operator!=(const T *ptr)
	{
		return !operator==(ptr);
	}

	element_type& operator[](size_t index)
	{
		return p[index];
	}

	T *release()
	{
		auto ret = p;
		p = nullptr;
		return p;
	}

	~unique_ptr(void)
	{
		if(p)
			delete p;
	}

	T* get_data()
	{
		return p;
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
};

template <typename T, class ... Args>
shared_ptr<T> make_shared(Args && ... args)
{
	T *data = new T(args...);
	if(!data)
		return nullptr;

	shared_ptr<T> p(data);
	return p;
}

template <typename T, typename U>
shared_ptr<T> cast(const shared_ptr<U>& s)
{
	auto ref = s.__get_refc();
	shared_ptr<T> p{};
	p.__set_refc((refcount<T> *) ref);
	return p;
}

template <typename T, class ... Args>
unique_ptr<T> make_unique(Args && ... args)
{
	T *data = new T(args...);
	if(!data)
		return nullptr;

	unique_ptr<T> p(data);
	return p;
}

#endif
