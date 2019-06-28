/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_DEVICE_H
#define _CARBON_DEVICE_H

#include <carbon/lock.h>
#include <carbon/list.h>
class Driver;

class Device
{
private:
	const char *name;
	Driver *owner;
public:
	Device(const char *name, Driver *owner) : name(name), owner(owner) {}

	~Device()
	{
		delete name;
	}

	void SetOwner(Driver *driver)
	{
		assert(owner == nullptr);
		owner = driver;
	}
};

class Driver
{
private:
	const char *name;
	LinkedList<Device *> devices;
public:
	Driver(const char *name) : name(name) {}
	bool BindDevice(Device *device)
	{
		bool st = devices.Add(device);

		if(st == true)
		{
			device->SetOwner(this);
		}

		return st;
	}

	~Driver()
	{
		delete name;
	}
};

#endif