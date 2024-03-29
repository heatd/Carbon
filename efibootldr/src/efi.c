/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of efibootldr, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include <stddef.h>
#include <stdint.h>

#include <carbon/bootprotocol.h>

struct boot_info *boot_info = NULL;

void *acpi_get_rsdp(void);

void get_boot_info(struct boot_info *binfo)
{
	boot_info->rsdp = acpi_get_rsdp();
}

void append_module(struct module *module)
{
	if(!boot_info->modules)
	{
		boot_info->modules = module;
	}
	else
	{
		struct module *m = boot_info->modules;
		while(m->next) m = m->next;
		m->next = module;
	}
}

struct relocation **rp = NULL;
void append_relocation(struct relocation *reloc)
{
	if(!rp)
	{
		boot_info->relocations = reloc;
		rp = &reloc->next;
	}
	else
	{
		*rp = reloc;
		rp = &reloc->next;
	}
}

EFI_SYSTEM_TABLE *get_system_table(void)
{
	return boot_info->SystemTable;
}

unsigned int get_highest_bit(UINT32 u)
{
	unsigned int r = 0;
	for(int i = 0; i < 32; i++)
	{
		if(u & (1 << i))
			r = i;
	}

	return r;
}

#define max(x, y) ((x > y) ? x : y)

unsigned int gop_get_bpp(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info)
{
	if(info->PixelFormat == PixelBltOnly)
			return 0;
	if(info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
	{
		boot_info->fb.color.red_mask = 0x00ff0000;
		boot_info->fb.color.green_mask = 0x0000ff00;
		boot_info->fb.color.blue_mask = 0x000000ff;
		boot_info->fb.color.resv_mask = 0xff000000;
		boot_info->fb.color.red_shift = 16;
		boot_info->fb.color.green_shift = 8;
		boot_info->fb.color.blue_shift = 0;
		boot_info->fb.color.resv_shift = 24;
		return 32;
	}
	else if(info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
	{
		boot_info->fb.color.red_mask = 0x000000ff;
		boot_info->fb.color.green_mask = 0x0000ff00;
		boot_info->fb.color.blue_mask = 0x00ff0000;
		boot_info->fb.color.resv_mask = 0xff000000;
		boot_info->fb.color.red_shift = 0;
		boot_info->fb.color.green_shift = 8;
		boot_info->fb.color.blue_shift = 16;
		boot_info->fb.color.resv_shift = 24;
		return 32;
	}
	else if(info->PixelFormat == PixelBitMask)
	{
		unsigned int red_bit = get_highest_bit(info->PixelInformation.RedMask);
		unsigned int blue_bit = get_highest_bit(info->PixelInformation.BlueMask);
		unsigned int green_bit = get_highest_bit(info->PixelInformation.GreenMask);
		unsigned int resv_bit = get_highest_bit(info->PixelInformation.ReservedMask);

		unsigned int bpp = max(resv_bit, max(green_bit, max(red_bit, blue_bit)));
		boot_info->fb.color.red_mask = info->PixelInformation.RedMask;
		boot_info->fb.color.blue_mask = info->PixelInformation.BlueMask;
		boot_info->fb.color.green_mask = info->PixelInformation.GreenMask;
		boot_info->fb.color.resv_mask = info->PixelInformation.ReservedMask;

		boot_info->fb.color.red_shift = red_bit;
		boot_info->fb.color.green_shift = green_bit;
		boot_info->fb.color.blue_shift = blue_bit;
		boot_info->fb.color.resv_shift = resv_bit;
		return bpp + 1;
	}
}

int initialize_graphics(EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS st = 0;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
	EFI_GUID edid_guid = EFI_EDID_ACTIVE_PROTOCOL_GUID;
	EFI_GUID guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	st = SystemTable->BootServices->LocateProtocol(&guid, NULL, (void**) &gop);
	if(st != EFI_SUCCESS)
	{
		Print(L"Failed to get the GOP protocol - error code 0x%x\n", st);
		return -1;
	}

	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = gop->Mode;
#if 0
	UINT32 to_be_set = 0;
	/* We're picking the biggest graphics framebuffer out of them all (it's usually a good idea) */
	UINT32 total_space = 0;
	for(UINT32 i = 0; i < mode->MaxMode; i++)
	{
		UINTN size;
		UINTN bpp;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
		st = gop->QueryMode(gop, i, &size, &info);
		if(st != EFI_SUCCESS)
		{
			Print(L"Failed to get video mode %u - error code 0x%x\n", i, st);
			return -1;
		}
		
		Print(L"Pixel format: %lx\n", info->PixelFormat);
		Print(L"%u x %u\n", info->HorizontalResolution, info->VerticalResolution);

		/* Ignore non-linear video modes */
		if(info->PixelFormat == PixelBltOnly)
			continue;
		if(info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor || info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
			bpp = 32;
		else if(info->PixelFormat == PixelBitMask)
		{
			/* How should this work? I think this works kinda fine? I'm not sure we even want 
			 * to support non-32 bit pixel formats
			*/
			Print(L"Red: %x Green: %x Blue: %x Reserved: %x",
                          info->PixelInformation.RedMask,
                          info->PixelInformation.GreenMask,
                          info->PixelInformation.BlueMask,
                          info->PixelInformation.ReservedMask);
			bpp = 24;
		}
		UINT32 fb_size = info->HorizontalResolution * info->VerticalResolution * (bpp / 8);
		if(fb_size > total_space)
		{
			to_be_set = i;
			total_space = fb_size;
		}
	}
#endif
	mode = gop->Mode;

	boot_info->fb.framebuffer = mode->FrameBufferBase;
	boot_info->fb.framebuffer_size = mode->FrameBufferSize;
	boot_info->fb.height = mode->Info->VerticalResolution;
	boot_info->fb.width = mode->Info->HorizontalResolution;

	boot_info->fb.bpp = gop_get_bpp(mode->Info);
	boot_info->fb.pitch = mode->Info->PixelsPerScanLine * (boot_info->fb.bpp/8);
	
	if(boot_info->fb.bpp == 0)
	{
		Print(L"efibootldr: Invalid graphics mode!\n");
		while(1);
	}

	return 0;
}

void *elf_load(void *buffer, size_t size);

UINT64 get_time(EFI_RUNTIME_SERVICES *rs)
{
	// this is not good
	EFI_TIME t0;
	UINT64 timestamp;
	EFI_STATUS st = rs->GetTime(&t0, NULL);

	if (EFI_ERROR(st))
	{
		return 0;
	}

	timestamp = t0.Nanosecond + t0.Second * 1000000000 + t0.Minute * 6000000000 +
	            t0.Hour * 6000000000 * 3600;
	
	return timestamp;
}

void *load_kernel(EFI_FILE *root, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS st;
	EFI_FILE *kernel;
	EFI_GUID file_info = EFI_FILE_INFO_ID;
	void *buffer;
	UINTN size;
	EFI_FILE_INFO *info;
	UINT64 t0, t1;

	EFI_RUNTIME_SERVICES *rs = SystemTable->RuntimeServices;

	t0 = get_time(rs);

	if((st = root->Open(root, &kernel, L"carbon", EFI_FILE_MODE_READ, 0)) != EFI_SUCCESS)
	{
		Print(L"Error: Open(): Could not open carbon - error code 0x%x\n", st);
		return NULL;
	}

	t1 = get_time(rs);

	Print(L"Open took %lu ms\n", (t1 - t0) / 1000000);

	if((st = SystemTable->BootServices->AllocatePool(EfiLoaderData,
		sizeof(EFI_FILE_INFO), (void**) &info)) != EFI_SUCCESS)
	{
		Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
		return NULL;
	}

	size = sizeof(EFI_FILE_INFO);
retry:
	if((st = kernel->GetInfo(kernel, &file_info, &size, (void*) info)) != EFI_SUCCESS)
	{
		if(st == EFI_BUFFER_TOO_SMALL)
		{
			SystemTable->BootServices->FreePool(info);
			if((st = SystemTable->BootServices->AllocatePool(
				EfiLoaderData, size, (void**) &info)) != EFI_SUCCESS)
			{
				Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
				return NULL;
			}
			goto retry;
		}
		Print(L"Error: GetInfo: Could not get file info - error code 0x%x\n", st);
		return NULL;
	}

	size = info->FileSize;
	if((st = SystemTable->BootServices->AllocatePool(EfiLoaderData, size, &buffer)) != EFI_SUCCESS)
	{
		SystemTable->BootServices->FreePool(info);
		Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
		return NULL;
	}

	t0 = get_time(rs);

	if((st = kernel->Read(kernel, &size, buffer)) != EFI_SUCCESS)
	{
		Print(L"Error: Read: Could not read the file - error code 0x%x\n", st);
		SystemTable->BootServices->FreePool(info);
		SystemTable->BootServices->FreePool(buffer);
		return NULL;
	}

	t1 = get_time(rs);

	Print(L"Read of %lu bytes took %lu ms\n", size, (t1 - t0) / 1000000);
	while(TRUE) {}
	void *entry_point = elf_load(buffer, size);
	return entry_point;
}

unsigned char entropy[NR_ENTROPY];

#ifdef __x86_64__
static inline unsigned long long rdtsc(void)
{
	unsigned long long ret = 0;
    	__asm__ __volatile__ ( "rdtsc" : "=A"(ret) );
    	return ret;
}
#endif

const int months[] = 
{
	31,
	28, 
	31,
	30,
	31,
	30,
	31,
	31,
	30,
	31,
	30,
	31
};

uint64_t get_unix_time(int year, int month, int day, int hour, int minute, int second)
{
	uint64_t utime = 0;
	for(int i = 1970; i < year; i++)
	{
		if(i % 100 == 0)
		{
			utime += 365 * 24 * 60 * 60;
		}
		else if (i % 400 == 0)
		{
			utime += 366 * 24 * 60 * 60; 
		}
		else if(i % 4 == 0)
		{
			utime += 366 * 24 * 60 * 60;
		}
		else
		{
			utime += 365 * 24 * 60 * 60; 
		}
	}
	// Calculate this year's POSIX time
	int total_day = 0;
	for(int m = 0; m < month-1; m++)
	{
		total_day += months[m];
	}
	total_day += day;
	if (year % 400 == 0)
	{
		total_day++; 
	}
	else if(year % 4 == 0)
	{
		total_day++;
	}
	utime += total_day * 86400;
	utime += hour * 60 * 60;
	utime += minute * 60;
	utime += second;

	return utime;
}

void initialize_entropy(EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_GUID guid = EFI_RNG_PROTOCOL_GUID;
	EFI_STATUS st;
	EFI_RNG_PROTOCOL *randomness;
	if((st = SystemTable->BootServices->LocateProtocol(&guid, NULL,
		(void**) &randomness)) != EFI_SUCCESS)
	{
		/* Fallback to GetTime() */
	fallback: ;
		EFI_TIME time;
		SystemTable->RuntimeServices->GetTime(&time, NULL);
		
		/* BUG: Unfortunately, some BIOSes(OVMF included) can't supply
		 * correct nanosecond times, and just return 0. Because of that, we can't use it.
		*/
		//CopyMem(&entropy[0], &time.Nanosecond, 4);
		entropy[0] = time.Day;
		entropy[1] = time.Hour;
		entropy[2] = time.Minute;
		entropy[3] = get_unix_time(time.Year, time.Month, time.Day,
			time.Hour, time.Minute, time.Second) & 0xff;

		#ifdef __x86_64__
		unsigned long long tsc = rdtsc();
		CopyMem(&entropy[4], &tsc, 8);

		if(tsc % 2)
			for(int i = 0; i < 10; i++) __asm__ __volatile__("pause");
		
		tsc = rdtsc();

		entropy[2] = tsc & 0xff;
		#endif
		return;
	}

	if((st = randomness->GetRNG(randomness, NULL, NR_ENTROPY, (void*) &entropy))
		!= EFI_SUCCESS)
	{
		Print(L"GetRNG failed: Error code %lx\n", st);
		goto fallback;
	}

}

int efi_exit_firmware(void *entry, EFI_SYSTEM_TABLE *SystemTable, EFI_HANDLE ImageHandle)
{
	EFI_STATUS st;
	UINTN nr_entries;
	UINTN key;
	UINTN desc_size;
	UINT32 desc_ver;
	
	Print(L"Brace yourselves, exiting firmware!\n");
	EFI_MEMORY_DESCRIPTOR *map = LibMemoryMap(&nr_entries, &key, &desc_size, &desc_ver);
	if(!map)
	{
		Print(L"Error: LibMemoryMap: Could not get the memory map\n");
		return -1;
	}
	boot_info->mmap.desc_ver = desc_ver;
	boot_info->mmap.descriptors = map;
	boot_info->mmap.nr_descriptors = nr_entries;
	boot_info->mmap.size_descriptors = desc_size;

	if((st = SystemTable->BootServices->ExitBootServices(ImageHandle, key)) != EFI_SUCCESS)
	{
		Print(L"Error: ExitBootServices: Could not exit the boot services - error code 0x%x\n", st);
		return -1;
	}

	void (*entry_point)(struct boot_info *boot) = (void (*)(struct boot_info*)) entry;
	entry_point(boot_info);
	return 0;
}

int load_initrd(EFI_FILE *root, EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS st;
	EFI_GUID guid = LOADED_IMAGE_PROTOCOL;
	EFI_GUID device_protocol = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_LOADED_IMAGE *image;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
	EFI_FILE_PROTOCOL *root;
	InitializeLib(ImageHandle, SystemTable);

	if(SystemTable->BootServices->AllocatePool(EfiLoaderData,
		sizeof(struct boot_info), (void**) &boot_info) != EFI_SUCCESS)
		return EFI_LOAD_ERROR;

	SetMem(boot_info, sizeof(struct boot_info), 0);
	boot_info->SystemTable = SystemTable;
	boot_info->modules = NULL;

	get_boot_info(boot_info);

	if(initialize_graphics(SystemTable) < 0)
		return EFI_LOAD_ERROR;
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	Print(L"efibootldr - booting...\n");

	initialize_entropy(SystemTable);
	
	/* Get a protocol to the loaded image */
	if((st = SystemTable->BootServices->HandleProtocol(ImageHandle, &guid, 
		(void**) &image)) != EFI_SUCCESS)
	{
		Print(L"Error: HandleProtocol(): Failed to get EFI_LOADED_IMAGE_PROTOCOL_GUID - error code 0x%x\n", st);
		return EFI_LOAD_ERROR;
	}
	boot_info->command_line = image->LoadOptions;
	/* Get a protocol to the loaded image */
	if((st = SystemTable->BootServices->HandleProtocol(image->DeviceHandle,
		&device_protocol, (void**) &fs)) != EFI_SUCCESS)
	{
		Print(L"Error: HandleProtocol(): Failed to get EFI_DEVICE_IO_PROTOCOL_GUID - error code 0x%x\n", st);
		return EFI_LOAD_ERROR;
	}
	if((st = fs->OpenVolume(fs, &root)) != EFI_SUCCESS) 
	{
		Print(L"Error: OpenVolume: Failed to open the root file - error code 0x%x\n", st);
		return EFI_LOAD_ERROR;
	}

	Print(L"Loading initrd\n");
	void *entry_point = NULL;
	if(load_initrd(root, SystemTable) < 0)
		return EFI_LOAD_ERROR;

	CopyMem(&boot_info->entropy, entropy, NR_ENTROPY);

	Print(L"Loading kernel\n");
	if((entry_point = load_kernel(root, SystemTable)) == NULL)
		return EFI_LOAD_ERROR;
	/* Exit the firmware and run the kernel */
	if(efi_exit_firmware(entry_point, SystemTable, ImageHandle) < 0)
		return EFI_LOAD_ERROR;
	for(;;) __asm__("hlt");
}
