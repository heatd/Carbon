/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/fs/file.h>

cbn_status_t file::seek(ssize_t off, unsigned long type, size_t *ret)
{
	if(!seekable)
		return CBN_STATUS_INVALID_ARGUMENT;
	
	switch(type)
	{
		case CBN_SEEK_ABS:
		{
			offset = (size_t) off;
			break;
		}
		case CBN_SEEK_CURR:
		{
			offset += off;
			break;
		}
		case CBN_SEEK_END:
		{
			offset = ino->i_size + off;
			break;
		}
		default:
			return CBN_STATUS_INVALID_ARGUMENT;
	}

	return CBN_STATUS_OK;
}

cbn_status_t file::write(const void *buffer, size_t len, size_t *written)
{
	ssize_t ret = 0;
	if((ret = ino->write(buffer, len, offset)) < 0)
	{
		return errno_to_cbn_status_t(errno);
	}

	*written = ret;
	offset += ret;
	return CBN_STATUS_OK;
}

cbn_status_t file::read(void *buffer, size_t len, size_t *read)
{
	ssize_t ret = 0;
	if((ret = ino->read(buffer, len, offset)) < 0)
	{
		return errno_to_cbn_status_t(errno);
	}

	*read = ret;
	offset += ret;
	return CBN_STATUS_OK;
}