/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PLATFORM_H
#define _CARBON_PLATFORM_H

#include <carbon/irq.h>

namespace Platform
{

namespace Irq
{

void UnmaskLine(::Irq::IrqLine line);
void MaskLine(::Irq::IrqLine line);

};

}
#endif