/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_FS_FILE_H
#define _CARBON_FS_FILE_H

#include <carbon/inode.h>
#include <carbon/refcount.h>

#define CBN_SEEK_ABS		(0)
#define CBN_SEEK_CURR		(1)
#define CBN_SEEK_END		(2)
class file : public refcountable
{
private:
	size_t offset;
	bool seekable;
	inode *ino;
public:
	file(bool _seekable, inode *ino) : seekable(_seekable), ino(ino) {}
	~file()
	{
		ino->close();
	}

	size_t get_offset()
	{
		return offset;
	}

	bool is_seekable()
	{
		return seekable;
	}

	cbn_status_t seek(ssize_t off, unsigned long type, size_t *ret);
	cbn_status_t write(const void *buffer, size_t len, size_t *written);
	cbn_status_t read(void *buffer, size_t len, size_t *read);
};

#endif