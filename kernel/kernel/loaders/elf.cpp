/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/loader.h>
#include <elf.h>
#include <carbon/vector.h>
#include <carbon/vm.h>

class elf_loader : public program_loader::binary_loader
{
private:
	static constexpr size_t elf_signature_size = 4;
	static constexpr unsigned char elf_curr_version = EV_CURRENT;
	
	static constexpr unsigned char elf_endianess = 
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		ELFDATA2LSB;
	#else
		ELFDATA2MSB;
	#endif

	const array<uint8_t, elf_signature_size> signature;
	cbn_status_t get_phdrs(const Elf64_Ehdr *file_header,
			       fs::auto_inode& ino, vector<Elf64_Phdr>& buf) const;
public:
	/* binary_loader interface */
	bool match(const array<uint8_t, program_loader::sample_size>& buf) const override;
	cbn_status_t load(fs::auto_inode& file, program_loader::binary_info& out) const override;
	/* end */
	elf_loader() : signature{ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3}
	{
		assert(program_loader::add_loader(this) == true);
	}
};

static elf_loader __elf_loader{};

cbn_status_t elf_loader::get_phdrs(const Elf64_Ehdr *file_header, fs::auto_inode& ino,
				   vector<Elf64_Phdr>& buf) const
{
	auto size = file_header->e_phnum * file_header->e_phentsize;

	if(!buf.alloc_buf(size))
		return CBN_STATUS_OUT_OF_MEMORY;
	auto buffer = buf.get_buf();

	auto count = fs::read((void *) buffer, size, file_header->e_phoff, ino);

	if(count != size)
	{
		return errno_to_cbn_status_t(errno);
	}

	buf.set_nr_elems(file_header->e_phnum);

	return CBN_STATUS_OK;
}

bool elf_loader::match(const array<uint8_t, program_loader::sample_size>& buf) const
{
	const Elf64_Ehdr *file_header = (const Elf64_Ehdr *) buf.data;
	for(unsigned int i = 0; i < elf_signature_size; i++)
	{
		if(file_header->e_ident[i] != signature[i])
			return false;
	}

	/* TODO: Add 32-bit loader */
	if(file_header->e_ident[EI_CLASS] != ELFCLASS64)
		return false;

	if(file_header->e_ident[EI_DATA] != elf_endianess)
		return false;

	if(file_header->e_ident[EI_VERSION] != elf_curr_version)
		return false;

	if(file_header->e_ident[EI_OSABI] != ELFOSABI_SYSV)
		return false;
	if(file_header->e_ident[EI_ABIVERSION] != 0)
		return false;

	return true;
}

cbn_status_t elf_loader::load(fs::auto_inode& file, program_loader::binary_info& out) const
{
	unique_ptr<Elf64_Ehdr> header = make_unique<Elf64_Ehdr>();
	if(!header)
		return CBN_STATUS_OUT_OF_MEMORY;

	auto count = fs::read((void *) header.get_data(), sizeof(Elf64_Ehdr), 0, file);
	if(count != sizeof(Elf64_Ehdr))
	{
		return errno_to_cbn_status_t(errno);
	}

	vector<Elf64_Phdr> phdrs{};

	auto status = get_phdrs(const_cast<const Elf64_Ehdr *> (header.get_data()), file, phdrs);
	if(status != CBN_STATUS_OK)
	{
		printf("debug: status %i\n", status);
		return status;
	}

	for(auto phdr : phdrs)
	{
		auto aligned_address = phdr.p_vaddr & ~(PAGE_SIZE - 1);
		auto misalignment = phdr.p_vaddr & (PAGE_SIZE - 1);
		auto mapping_off = phdr.p_offset - misalignment;
		bool is_bss = phdr.p_filesz != phdr.p_memsz;
		if(mapping_off > phdr.p_offset)
		{
			/* undeflow happened */
			return CBN_STATUS_INVALID_ARGUMENT;
		}
		
		/* This is impossible and seems like a bad */
		if(phdr.p_memsz < phdr.p_filesz)
		{
			return CBN_STATUS_OUT_OF_MEMORY;
		}

		auto size = phdr.p_memsz + misalignment;
		auto prot = ((phdr.p_flags & PF_W) ? VM_PROT_WRITE : 0) |
			    ((phdr.p_flags & PF_X) ? VM_PROT_EXEC : 0) |
			    VM_PROT_USER;
	
		void *addr = Vm::map_file(&out.proc->address_space,
					  (void *) aligned_address,
			     		  MAP_FILE_FIXED, prot, size, mapping_off, file);
		if(!addr)
		{
			return CBN_STATUS_OUT_OF_MEMORY;
		}

		if(is_bss)
		{
			uint8_t *bss_base = (uint8_t *) (phdr.p_vaddr + phdr.p_filesz);
			size_t bss_size = phdr.p_memsz - phdr.p_filesz;
			
			cbn_status_t st = CBN_STATUS_OK;
	
			out.proc->set_memory(bss_base, 0, bss_size, st);
			if(st != CBN_STATUS_OK)
				return st;
		}

		printf("base %lx - size %lx\n", phdr.p_vaddr, phdr.p_memsz);
	}

	out.entry = (program_loader::binary_entry_point_t) header->e_entry;
	return CBN_STATUS_OK;
}