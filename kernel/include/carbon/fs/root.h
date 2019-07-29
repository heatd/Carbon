/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_FS_ROOT_H
#define _CARBON_FS_ROOT_H

#include <carbon/dentry.h>

dentry* get_root();
void set_root(dentry* dentry);

#endif