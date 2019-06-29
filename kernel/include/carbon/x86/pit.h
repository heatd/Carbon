/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_X86_PIT_H
#define _CARBON_X86_PIT_H

#include <stdint.h>

namespace Pit
{

void InitOneshot(uint32_t freq);
void WaitForOneshot();

}
#endif