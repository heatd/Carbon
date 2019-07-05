/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_REGISTERS_H
#define _CARBON_REGISTERS_H

#ifdef __x86_64__
#include <carbon/x86/registers.h>
#else
#error "struct registers not implemented"
#endif

#endif