/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <carbon/loader.h>
#include <elf.h>
#include <carbon/vector.h>
#include <carbon/vm.h>
#include <carbon/process_args.h>
#include <carbon/syscall_utils.h>

class elf_loader : public program_loader::binary_loader
{
private:
	static constexpr size_t elf_signature_size = 4;
	static constexpr unsigned char elf_curr_version = EV_CURRENT;
	static constexpr size_t auxv_entries = 38;
	static constexpr unsigned char elf_endianess = 
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		ELFDATA2LSB;
	#else
		ELFDATA2MSB;
	#endif

	const array<uint8_t, elf_signature_size> signature;
	cbn_status_t get_phdrs(const Elf64_Ehdr *file_header,
			       fs::auto_inode& ino, cul::vector<Elf64_Phdr>& buf) const;
	static void destroy_auxv(void *_auxv);
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
				   cul::vector<Elf64_Phdr>& buf) const
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

void elf_loader::destroy_auxv(void *_auxv)
{
	Elf64_auxv_t *auxv = (Elf64_auxv_t *) _auxv;
	delete [] auxv;
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

	cul::vector<Elf64_Phdr> phdrs{};

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
		
		if(phdr.p_type != PT_LOAD)
			continue;
		
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
		/* TODO: Sanitize addresses */
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
			{
				return st;
			}
		}

		//printf("base %lx - size %lx\n", phdr.p_vaddr, phdr.p_memsz);
		cbn_status_t st;
		size_t p = 0;
		out.proc->read_memory((void *) 0x402fe0, (void *) &p, sizeof(size_t), st);
		//printf("P: %lx\n", p);
	}

	//printf("allocating auxv\n");

	Elf64_auxv_t *auxv = new Elf64_auxv_t[auxv_entries];

	if(!auxv)
		return CBN_STATUS_OUT_OF_MEMORY;

	size_t scratch_pages_size = strlen(out.proc->get_name()) + 1 +	// EXECFN
				    16;					// RANDOM

	auto scratch_page = Vm::mmap(&out.proc->address_space, 0, scratch_pages_size, VM_PROT_WRITE
				| VM_PROT_USER);
	if(!scratch_page)
	{
		delete auxv;
		return CBN_STATUS_OUT_OF_MEMORY;
	}

	char *scratch_space = (char *) scratch_page;

	for(int i = 0; i < 38; i++)
	{
		if(i != 0)
			auxv[i].a_type = i;
		else
			auxv[i].a_type = 0xffff;
		if(i == 37)
			auxv[i].a_type = AT_NULL;
		switch(i)
		{
			case AT_PAGESZ:
				auxv[i].a_un.a_val = PAGE_SIZE;
				break;
			case AT_UID:
				//auxv[i].a_un.a_val = process->uid;
				break;
			case AT_GID:
				//auxv[i].a_un.a_val = process->gid;
				break;
			case AT_RANDOM:
				//get_entropy((char*) scratch_space, 16);
				//scratch_space += 16;
				break;
			case AT_BASE:
				//auxv[i].a_un.a_val = (uintptr_t) process->image_base;
				break;
			case AT_PHENT:
				//auxv[i].a_un.a_val = process->info.phent;
				break;
			case AT_PHNUM:
				//auxv[i].a_un.a_val = process->info.phnum;
				break;
			case AT_PHDR:
				//auxv[i].a_un.a_val = (uintptr_t) process->info.phdr;
				break;
			case AT_EXECFN:
			{
				auxv[i].a_un.a_val = (uintptr_t) scratch_space;

				size_t len = strlen(out.proc->get_name()) + 1;
				cbn_status_t st = CBN_STATUS_OK;
				out.proc->write_memory((char*) scratch_space,
					(void *) out.proc->get_name(), len, st);
				if(st != CBN_STATUS_OK)
				{
					delete auxv;
					return st;
				}

				scratch_space += len;
				break;
			}
			case AT_SYSINFO_EHDR:
				//auxv[i].a_un.a_val = (uintptr_t) process->vdso;
				break;
		}
	}

	out.entry = (program_loader::binary_entry_point_t) header->e_entry;
	out.args->set_kbin_args(auxv);
	out.args->set_kbin_arg_size(auxv_entries * sizeof(Elf64_auxv_t));
	out.args->set_kbin_dtor(destroy_auxv);

	return CBN_STATUS_OK;
}