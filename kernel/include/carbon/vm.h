/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _CARBON_VM_H
#define _CARBON_VM_H

#include <stddef.h>
#include <stdint.h>

#include <carbon/lock.h>

#ifdef __x86_64__
#include <carbon/x86/vm.h>
#endif

#include <carbon/address_space.h>
#include <carbon/process.h>


extern struct address_space kernel_address_space;

#define kernel_as &kernel_address_space

#define VM_PROT_WRITE		(1 << 0)
#define VM_PROT_EXEC		(1 << 1)
#define VM_PROT_USER		(1 << 2)

int vm_cmp(const void* k1, const void* k2);

class vm_object;

struct vm_region
{
	unsigned long start;
	size_t size;
	unsigned long perms;
	vm_object *vmo;
	unsigned long off;
};

void vm_init(void);
struct vm_region *vm_allocate_region(struct address_space *as,
				     unsigned long min, size_t size);
struct vm_region *vm_reserve_region(struct address_space *as,
				    unsigned long start, size_t size);

namespace Vm
{

enum VmFaultStatus
{
	VM_OK = 0,
	VM_SEGFAULT = 1,
	VM_SIGBUS = 2
};

class VmFault
{
protected:
	bool is_user;
	bool is_write;
	bool is_exec;
	unsigned long fault_address;
	unsigned long ip;
	unsigned long FlagsToPerms();
	enum VmFaultStatus TryToMapPage(struct vm_region *region);
public:
	VmFault(bool is_user, bool is_write, bool is_exec,
		unsigned long fault_address, unsigned long ip) : is_user(is_user),
		is_write(is_write), is_exec(is_exec), fault_address(fault_address),
		ip(ip) {}
	~VmFault() {}

	enum VmFaultStatus Handle();
	void Dump();
};

struct vm_region *AllocateRegionInternal(struct address_space *as, unsigned long min,
					 size_t size);
struct vm_region *MmapInternal(struct address_space *as, unsigned long min, size_t size,
			       unsigned long flags, vm_object *vmo);
void *mmap(struct address_space *as, unsigned long min, size_t size, unsigned long flags);
void *MmioMap(struct address_space *as, unsigned long phys, unsigned long min,
	      size_t size, unsigned long flags);
int munmap(struct address_space *as, void *addr, size_t size);

void ForEveryRegion(struct address_space *as, bool (*)(struct vm_region *region));

void *allocate_stack(struct address_space *as, size_t size, unsigned long flags);

inline struct address_space *get_current_address_space()
{
	if(unlikely(!Percpu::percpu_initialized()))
		return kernel_as;
	auto p = get_current_process();
	if(likely(p))
		return &p->address_space;
	else
	{
		return kernel_as;
	}
}

struct vm_region *FindRegion(void *addr, rb_tree *tree);

#define MAP_FILE_FIXED		(1 << 0)
#define MAP_FILE_SHARED		(1 << 1)
#define MAP_FILE_PRIVATE	(1 << 2)

void *map_file(struct address_space *as, void *addr_hint,
		      unsigned long flags, unsigned long prot,
		      size_t size, size_t off, inode *ino);

};

void *vm_create_page_tables();

#ifdef __x86_64__
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#endif

#endif