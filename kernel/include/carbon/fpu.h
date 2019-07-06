/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_FPU_H
#define _CARBON_FPU_H

#ifdef __x86_64__

#define FPU_AREA_ALIGNMENT 	64
#define FPU_AREA_SIZE		2048

#endif

namespace Fpu
{

void SaveFpu(unsigned char *area);
void RestoreFpu(unsigned char *area);
void Init();
void SetupFpuArea(unsigned char *area);

}

#endif