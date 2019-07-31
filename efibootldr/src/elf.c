/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of efibootldr, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stddef.h>
#include <stdbool.h>
#include <elf.h>
#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include <carbon/bootprotocol.h>

void append_relocation(struct relocation *reloc);

#define add_check_overflow(op1, op2, res) __builtin_add_overflow(op1, op2, res)

bool elf_check_off(Elf64_Off off, size_t buf_size)
{
	if(buf_size < off)
		return false;
	return true;
}

bool elf_is_valid(Elf64_Ehdr *header)
{
	if (header->e_ident[EI_MAG0] != 0x7F || header->e_ident[EI_MAG1] != 'E' || header->e_ident[EI_MAG2] != 'L' || header->e_ident[EI_MAG3] != 'F')
		return false;
	if (header->e_ident[EI_CLASS] != ELFCLASS64)
		return false;
	if (header->e_ident[EI_DATA] != ELFDATA2LSB)
		return false;
	if (header->e_ident[EI_VERSION] != EV_CURRENT)
		return false;
	if (header->e_ident[EI_OSABI] != ELFOSABI_SYSV)
		return false;
	if (header->e_ident[EI_ABIVERSION] != 0)	/* SYSV specific */
		return false;
	return true;
}

char *elf_get_string(Elf64_Word off, Elf64_Shdr *shtab, Elf64_Ehdr *hdr)
{
	return (char*) hdr + shtab->sh_offset + off;
}

int __strcmp(const char *s, const char *t)
{
	int i;
	for (i = 0; s[i] == t[i]; i++)
		if (s[i] == '\0')
			return 0;
	return s[i] - t[i];
}

Elf64_Sym *elf_get_sym(Elf64_Ehdr *hdr, const char *sym, bool needs_strong)
{
	Elf64_Shdr *tab = (Elf64_Shdr*)((char*) hdr + hdr->e_shoff);
	size_t nr_entries = hdr->e_shnum;
	Elf64_Shdr *shstr = &tab[hdr->e_shstrndx];
	Elf64_Shdr *symtab = NULL, *strtab = NULL;
	for(size_t i = 0; i < nr_entries; i++)
	{
		if(!__strcmp(elf_get_string(tab[i].sh_name, shstr, hdr), ".symtab"))
			symtab = &tab[i];
		if(!__strcmp(elf_get_string(tab[i].sh_name, shstr, hdr), ".strtab"))
			strtab = &tab[i];
	}

	nr_entries = symtab->sh_size / symtab->sh_entsize;
	Elf64_Sym *syms = (Elf64_Sym*) ((char*) hdr + symtab->sh_offset);

	for(size_t i = 0; i < nr_entries; i++)
	{
		if(!__strcmp(elf_get_string(syms[i].st_name, strtab, hdr), sym))
		{
			if(needs_strong && ELF64_ST_BIND(syms[i].st_info) & STB_WEAK)
			{
				continue;
			}

			return &syms[i];
		}
	}

	return NULL;
}

Elf64_Sym *elf_get_sym_from_index(Elf64_Ehdr *hdr, size_t idx)
{
	Elf64_Shdr *tab = (Elf64_Shdr*)((char*) hdr + hdr->e_shoff);
	size_t nr_entries = hdr->e_shnum;
	Elf64_Shdr *shstr = &tab[hdr->e_shstrndx];
	Elf64_Shdr *symtab = NULL, *strtab = NULL;
	
	for(size_t i = 0; i < nr_entries; i++)
	{
		if(!__strcmp(elf_get_string(tab[i].sh_name, shstr, hdr), ".symtab"))
			symtab = &tab[i];
		if(!__strcmp(elf_get_string(tab[i].sh_name, shstr, hdr), ".strtab"))
			strtab = &tab[i];
	}

	Elf64_Sym *syms = (Elf64_Sym*) ((char*) hdr + symtab->sh_offset);
	Elf64_Sym *sym = &syms[idx];
	if(ELF64_ST_BIND(sym->st_info) & STB_WEAK && sym->st_shndx == STN_UNDEF)
		sym = elf_get_sym(hdr, elf_get_string(syms[idx].st_name, strtab, hdr), true);
	return sym ? sym : &syms[idx];
}

EFI_SYSTEM_TABLE *get_system_table(void);

struct serial_port
{
	uint16_t io_port;
	int com_nr;
};

static inline void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val)
{
	__asm__ __volatile__ ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val)
{
	__asm__ __volatile__ ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ __volatile__ ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
	return ret;
}

struct serial_port com1 = {.io_port = 0x3f8, .com_nr = 0};
bool serial_is_transmit_empty(struct serial_port *port)
{
	return inb(port->io_port + 5) & 0x20;
}

bool serial_recieved(struct serial_port *port)
{
	return inb(port->io_port + 5) & 1;
}

static void write_byte(char c, struct serial_port *port)
{
	while(!serial_is_transmit_empty(port));

	outb(port->io_port, c);
	if(c == '\n')
		write_byte('\r', port);
}

void serial_write(const char *s, size_t size, struct serial_port *port)
{
	for(size_t i = 0; i < size; i++)
	{
		write_byte(*(s + i), port);
	}
}

void fark(char *s)
{
	serial_write(s, strlena(s), &com1);
}

static char read_byte(struct serial_port *port)
{
	while(!serial_recieved(port));

	return inb(port->io_port);
}

void serial_port_init(struct serial_port *port)
{
	outb(port->io_port + 1, 0x00);    // Disable all interrupts
	outb(port->io_port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(port->io_port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(port->io_port + 1, 0x00);    //                  (hi byte)
	outb(port->io_port + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(port->io_port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(port->io_port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void elf_do_reloc(Elf64_Ehdr *header, Elf64_Rela *rela)
{
	size_t sym_idx = ELF64_R_SYM(rela->r_info);
	Elf64_Shdr *tab = (Elf64_Shdr*)((char*) header + header->e_shoff);
	Elf64_Shdr *shstr = &tab[header->e_shstrndx];
	if(sym_idx != SHN_UNDEF)
	{
		Elf64_Sym *sym = elf_get_sym_from_index(header, sym_idx);
		Elf64_Shdr *section = &tab[sym->st_shndx];
		char *name = elf_get_string(section->sh_name, shstr, header);
	
		if(!__strcmp(elf_get_string(section->sh_name, shstr, header), ".percpu"))
		{
			return;
		}
	
		EFI_STATUS st;
		EFI_SYSTEM_TABLE *SystemTable = get_system_table();
		struct relocation *rel;
		if((st = SystemTable->BootServices->AllocatePool(EfiLoaderData,
		sizeof(struct relocation), (void**) &rel)) != EFI_SUCCESS)
		{
			Print(L"Error: AllocatePool: Could not get memory - error code 0x%x\n", st);
			return;
		}

		rel->symbol_value = sym->st_value;
		rel->type = ELF64_R_TYPE(rela->r_info);
		rel->addend = rela->r_addend;
		rel->address = (unsigned long *) rela->r_offset;
		rel->next = NULL;

		append_relocation(rel);
	}
}

void elf_parse_rela(Elf64_Ehdr *header, Elf64_Rela *relocs, size_t entries)
{
	for(size_t i = 0; i < entries; i++)
	{
		/* TODO: Maybe some error checking would be nice? */
		elf_do_reloc(header, &relocs[i]);
	}
}

bool section_should_relocate(Elf64_Shdr *target_section, Elf64_Shdr *shstr, Elf64_Ehdr *hdr)
{
	const char *should_not_reloc[] = {".boot", ".percpu"};
	const size_t nr_entries = sizeof(should_not_reloc) / sizeof(char *);

	const char *section_name = elf_get_string(target_section->sh_name, shstr, hdr);

	for(size_t i = 0; i < nr_entries; i++)
	{
		if(!__strcmp(should_not_reloc[i], section_name))
			return false;
	}
	
	serial_write("Relocating ", strlena("Relocating"), &com1);
	serial_write(section_name, strlena(section_name), &com1);
	serial_write("\n", 1, &com1);
	return true;
}

void elf_parse_relocations(Elf64_Ehdr *hdr)
{
	serial_port_init(&com1);
	Elf64_Shdr *tab = (Elf64_Shdr*)((char*) hdr + hdr->e_shoff);
	size_t nr_entries = hdr->e_shnum;
	Elf64_Shdr *shstr = &tab[hdr->e_shstrndx];

	for(size_t i = 0; i < nr_entries; i++)
	{
		Elf64_Shdr *rel_section = &tab[i];
		if(rel_section->sh_type == SHT_RELA)
		{
			Elf64_Shdr *target_section = &tab[rel_section->sh_info];

			if(!section_should_relocate(target_section, shstr, hdr))
				continue;
			elf_parse_rela(hdr, (Elf64_Rela*)((char*) hdr + tab[i].sh_offset),
				tab[i].sh_size / tab[i].sh_entsize);
		}
	}
}

void *elf_load(void *buffer, size_t size)
{
	if (!elf_is_valid((Elf64_Ehdr *) buffer))
	{
		Print(L"Invalid ELF64 file!\n");
		return NULL;
	}
	Elf64_Ehdr *hdr = (Elf64_Ehdr *) buffer;
	if(elf_check_off(hdr->e_phoff, size) == false)
	{
		Print(L"Invalid offset %x\n", hdr->e_phoff);
		return NULL;
	}
	Elf64_Phdr *phdr = (Elf64_Phdr *) ((char *) buffer + hdr->e_phoff);
	for(Elf64_Half i = 0; i < hdr->e_phnum; i++, phdr++)
	{
		if(phdr->p_type == PT_NULL)
			continue;
		if(phdr->p_type == PT_LOAD && phdr->p_vaddr != 0)
		{
			if(elf_check_off(phdr->p_offset, size) == false)
			{
				Print(L"Invalid offset %x\n", phdr->p_offset);
				return NULL;
			}
			if(elf_check_off(phdr->p_offset + phdr->p_filesz, size) == false)
			{
				Print(L"Invalid offset %x\n", phdr->p_offset + phdr->p_filesz);
				return NULL;
			}

			CopyMem((void*) phdr->p_paddr, (void*) ((char *) buffer + phdr->p_offset), phdr->p_filesz);
			SetMem((void*) (char*) phdr->p_paddr + phdr->p_filesz, phdr->p_memsz - phdr->p_filesz, 0);
		}
	}

	elf_parse_relocations(hdr);

	Elf64_Sym *sym = elf_get_sym(hdr, "efi_entry_point", false);
	if(!sym)
		return (void*) hdr->e_entry;
	else
		return (void*) sym->st_value;
}
