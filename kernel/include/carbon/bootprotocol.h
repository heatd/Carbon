/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _BOOTPROTOCOL_H
#define _BOOTPROTOCOL_H

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include <stddef.h>
#include <stdint.h>

#define NR_ENTROPY 32

struct color_info
{
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
	uint32_t resv_mask;
	uint32_t red_shift;
	uint32_t green_shift;
	uint32_t blue_shift;
	uint32_t resv_shift;
};

struct boot_framebuffer
{
	unsigned long framebuffer;
	unsigned long framebuffer_size;
	unsigned long height;
	unsigned long width;
	unsigned long bpp;
	unsigned long pitch;
	struct color_info color;
};

struct module
{
	char name[256];
	uint64_t start;
	uint64_t size;
	struct module *next;
};

struct efi_mmap_info
{
	EFI_MEMORY_DESCRIPTOR *descriptors;
	UINTN nr_descriptors;
	UINTN size_descriptors;
	UINTN desc_ver;
};

/*
 * Use a custom struct relocation that's semi-independent from ELF(so we don't
 * have to bring in the elf headers. Already has symbol values, types and patch addresses
*/

struct relocation
{
	unsigned long type;
	unsigned long symbol_value;
	unsigned long addend;
	unsigned long *address;
	struct relocation *next;
} __attribute__((packed));

/*
 * Use a custom struct symbol that's independent from ELF.
*/

struct symbol
{
	/* TODO: Maybe we shouldn't impose a limit on this? */
	char name[512];
	unsigned long value;
	struct symbol *next;
};

struct boot_info
{
	char entropy[NR_ENTROPY];
	struct relocation *relocations;
	struct boot_framebuffer fb;
	struct module *modules;
	EFI_SYSTEM_TABLE *SystemTable;
	struct efi_mmap_info mmap;
	wchar_t *command_line;
	struct symbol *symtab;

	/* ACPI RSDP */
	void *rsdp;
};

#ifdef __is_carbon_kernel

extern struct boot_info *boot_info;

#endif

#endif
