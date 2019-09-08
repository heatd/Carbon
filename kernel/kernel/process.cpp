/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <string.h>

#include <carbon/process.h>
#include <carbon/lock.h>
#include <carbon/vm.h>
#include <carbon/vmobject.h>
#include <carbon/loader.h>

#include <libdict/rb_tree.h>

Spinlock pnamespace_list_lock;
LinkedList<shared_ptr<process_namespace> > pnamespace_list;
static uint32_t next_pnamespace_id = 0;

process_namespace::~process_namespace(){}

shared_ptr<process_namespace> process_namespace::create_namespace(cbn_status_t& out_status)
{
	scoped_lock<Spinlock> guard{&pnamespace_list_lock};

	auto _namespace = make_shared<process_namespace>(next_pnamespace_id++);
	if(!_namespace)
		goto back_out;

	if(!pnamespace_list.Add(_namespace))
		goto back_out;

	out_status = CBN_STATUS_OK;
	return _namespace;
back_out:
	next_pnamespace_id--;
	out_status = CBN_STATUS_OUT_OF_MEMORY;
	return nullptr;
}

process *process_namespace::create_process_in_namespace(cbn_status_t& out_status)
{
	scoped_lock<Spinlock> guard{&process_creation_lock};

	if(process_limit != 0 && nr_processes + 1 == process_limit)
	{
		out_status = CBN_STATUS_RSRC_LIMIT_HIT;
		return nullptr;
	}

	process *p = new process(this, curr_pid++);
	if(!p)
	{
		out_status = CBN_STATUS_OUT_OF_MEMORY;
		return nullptr;
	}

	if(!process_list.Add(p))
	{
		delete p;
		out_status = CBN_STATUS_OUT_OF_MEMORY;
		return nullptr;
	}

	nr_processes++;

	return p;
}

bool process_namespace::remove_process(process *p)
{
	scoped_spinlock guard{&process_creation_lock};
	return process_list.Remove(p);
}

void process::abort_construction()
{
	pnamespace->remove_process(this);
	delete this;
}

bool process::initialize_address_space()
{
	address_space.start = (unsigned long) vm::limits::user_min;
	address_space.end = (unsigned long) vm::limits::user_max;
	address_space.area_tree = rb_tree_new(vm_cmp);

	if(!address_space.area_tree)
		return false;

	address_space.arch_priv = vm_create_page_tables();
	if(!address_space.arch_priv)
	{
		rb_tree_free(address_space.area_tree, nullptr);
		return false;
	}

	return true;
}

process *process::spawn(const char *name, unsigned long flags, shared_ptr<process_namespace> proc_namespace,
			cbn_status_t& out_status)
{
	bool starting_main_thread = flags & PROCESS_SPAWN_FLAG_START;
	bool creating_main_thread = flags & PROCESS_SPAWN_FLAG_CREATE_THREAD;
	bool early_exit = !starting_main_thread && !creating_main_thread;
	if(starting_main_thread && !(creating_main_thread))
	{
		out_status = CBN_STATUS_INVALID_ARGUMENT;
		return nullptr;
	}

	process *new_process = proc_namespace->create_process_in_namespace(out_status);
	if(!new_process)
		return nullptr;
	
	if(new_process->set_name(name) < 0)
	{
		new_process->abort_construction();
		out_status = CBN_STATUS_OUT_OF_MEMORY;
		return nullptr;
	}

	if(!new_process->initialize_address_space())
	{
		new_process->abort_construction();
		out_status = CBN_STATUS_OUT_OF_MEMORY;
		return nullptr;
	}

	/* If we don't need to spawn more threads, we're okay, so return */
	if(early_exit)
		return new_process;
#if 0
	thread *main_thread = nullptr;
	if(flags & PROCESS_SPAWN_FLAG_CREATE_THREAD)
	{
		size_t ustack_size = (size_t) vm::defaults::user_stack_size;
		void *ustack = Vm::allocate_stack(&new_process->address_space,
				ustack_size, VM_PROT_WRITE | VM_PROT_USER);
		if(!ustack)
		{
			new_process->abort_construction();
			return nullptr;
		} 
		printf("ustack: %p\n", ustack);
		if(!(main_thread = new_process->create_thread(data.callback, data.arg,
							      ustack, out_status)))
		{
			new_process->abort_construction();
			auto p = (void *) ((char *) ustack - ustack_size);
			Vm::munmap(&new_process->address_space, p, ustack_size);

			/* out_status has been set by create thread */
			return nullptr;
		}
	}

	if(flags & PROCESS_SPAWN_FLAG_START)
	{
		scheduler::start_thread(main_thread);
	}
#endif

	return new_process;
}

thread *process::create_thread(scheduler::thread_callback entry, void *arg, void *user_stack,
			       cbn_status_t& out_status)
{
	scoped_spinlock guard{&thread_lock};
	if(thread_limit != 0 && number_of_threads + 1 == thread_limit)
	{
		out_status = CBN_STATUS_RSRC_LIMIT_HIT;
		return nullptr;
	}

	thread *t = scheduler::create_thread(entry, arg, (scheduler::create_thread_flags) 0);
	if(!t)
	{
		out_status = CBN_STATUS_OUT_OF_MEMORY;
		return nullptr;
	}

	pair p{number_of_threads++, t};
	
	if(!thread_list.Add(p))
	{
		out_status = CBN_STATUS_OUT_OF_MEMORY;
		panic("TODO: Delete thread");
	}

	t->owner = this;
	t->get_registers()->rsp = (unsigned long) user_stack;
	return t;
}

size_t process::write_memory(void *address, void *src, size_t len, cbn_status_t& out_status)
{
	size_t written = 0;
	scoped_spinlock guard {&address_space.lock};

	unsigned long a = (unsigned long) address;
	const uint8_t *s = (const uint8_t *) src; 
	while(written != len)
	{
		auto region = Vm::FindRegion((void *) a, address_space.area_tree);
		if(!region)
		{
			out_status = CBN_STATUS_SEGFAULT;
			return written ? written : (-1);
		}
	
		size_t to_write = len < region->size ? len : region->size;
		auto region_offset = a - region->start;
		region->vmo->write(region->off + region_offset, s, to_write);
		written += to_write;
		a += to_write;
		s += to_write;
	}

	out_status = CBN_STATUS_OK;
	return written;
}

size_t process::set_memory(void *address, uint8_t pattern, size_t len, cbn_status_t& out_status)
{
	size_t written = 0;
	scoped_spinlock guard {&address_space.lock};

	unsigned long a = (unsigned long) address;

	while(written != len)
	{
		auto region = Vm::FindRegion((void *) a, address_space.area_tree);
		if(!region)
		{
			out_status = CBN_STATUS_SEGFAULT;
			return written ? written : (-1);
		}
	
		size_t to_write = len < region->size ? len : region->size;
		auto region_offset = a - region->start;
		region->vmo->set_mem(region->off + region_offset, pattern, to_write);
		written += to_write;
		a += to_write;
	}

	out_status = CBN_STATUS_OK;
	return written;
}

size_t process::read_memory(void *address, void *dest, size_t len, cbn_status_t& out_status)
{
	size_t been_read = 0;
	scoped_spinlock guard {&address_space.lock};

	unsigned long a = (unsigned long) address;
	uint8_t *d = (uint8_t *) dest;
	while(been_read != len)
	{
		auto region = Vm::FindRegion((void *) a, address_space.area_tree);
		if(!region)
		{
			out_status = CBN_STATUS_SEGFAULT;
			return been_read ? been_read : (-1);
		}
	
		size_t to_read = len < region->size ? len : region->size;
		auto region_offset = a - region->start;
		region->vmo->read(region->off + region_offset, d, to_read);
		been_read += to_read;
		a += to_read;
		d += to_read;
	}

	out_status = CBN_STATUS_OK;
	return been_read;
}

cbn_status_t process::load_binary(const char *path, program_loader::binary_info& info)
{
	return program_loader::load(path, this, info);
}

cbn_status_t process::start()
{
	scoped_spinlock guard{&thread_lock};
	if(thread_list.IsEmpty())
		return CBN_STATUS_INVALID_ARGUMENT;

	auto& first_thread_pair = *thread_list.begin();
	auto first_thread = first_thread_pair.second_member;

	scheduler::start_thread(first_thread);

	return CBN_STATUS_OK;
}

int process::set_name(const char *name)
{
	const char *process_name = strdup(name);
	if(!process_name)
		return -1;
	this->name = process_name;

	return 0;
}

const char *process::get_name() const
{
	return name;
}

process::process(shared_ptr<process_namespace> n, cbn_pid_t __pid)
: refcountable{1}, thread_list{}, name{}, pnamespace{n}, pid{__pid}, address_space{}
{
}

process::~process()
{
	free((void *) name);	
}

process *process::kernel_spawn_process_helper(const char *name, const char *path,
			unsigned long flags, shared_ptr<process_namespace> proc_namespace,
			cbn_status_t& out)
{
	auto p = spawn(name, flags, proc_namespace, out);
	if(!p)
		return nullptr;
	
	program_loader::binary_info info;
	auto st = p->load_binary(path, info);

	if(st != CBN_STATUS_OK)
		printf("Failure: Status %i\n", st);
	printf("Entry: %p\n", info.entry);
	auto ustack = Vm::allocate_stack(&p->address_space, (size_t) vm::defaults::user_stack_size,
		VM_PROT_WRITE | VM_PROT_USER);
	assert(ustack != nullptr);
	auto thread = p->create_thread((scheduler::thread_callback) info.entry, NULL, ustack, st);
	scheduler::start_thread(thread);

	return p;
}