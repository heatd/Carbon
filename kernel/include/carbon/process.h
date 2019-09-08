/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#ifndef _CARBON_PROCESS_H
#define _CARBON_PROCESS_H


#include <carbon/scheduler.h>
#include <carbon/list.h>
#include <carbon/handle_table.h>
#include <carbon/smart.h>
#include <carbon/pair.h>
#include <carbon/address_space.h>
#include <carbon/loader.h>

typedef uint32_t cbn_pid_t;

class process_namespace;
class process : public refcountable
{
private:
	using thread_id_t = unsigned long;
	LinkedList<pair<thread_id_t, thread*> > thread_list;
	thread_id_t number_of_threads;
	thread_id_t thread_limit;
	Spinlock thread_lock;
	const char *name;
	shared_ptr<process_namespace> pnamespace;
	cbn_pid_t pid;
public:
	struct address_space address_space;

	process(shared_ptr<process_namespace> pnamespace, cbn_pid_t pid);
	~process();

	int set_name(const char *string);
	const char *get_name() const;
	bool initialize_address_space();
	void abort_construction();

	#define PROCESS_SPAWN_FLAG_CREATE_THREAD		(1 << 0)
	#define PROCESS_SPAWN_FLAG_START			(1 << 1)
	#define PROCESS_SPAWN_LOAD_PROCESS			(1 << 2)

	static process *spawn(const char *name, unsigned long flags,
		shared_ptr<process_namespace> proc_namespace, cbn_status_t& out_status);
	static process *kernel_spawn_process_helper(const char *name, const char *path,
			unsigned long flags, shared_ptr<process_namespace> proc_namespace,
			cbn_status_t& out);
	thread *create_thread(scheduler::thread_callback entry, void *arg, void *user_stack,
			      cbn_status_t& out_status);
	cbn_status_t load_binary(const char *path, program_loader::binary_info& info);
	cbn_status_t start();
	size_t write_memory(void *address, void *src, size_t len, cbn_status_t& out_status);
	size_t read_memory(void *address, void *dest, size_t len, cbn_status_t& out_status);
	size_t set_memory(void *address, uint8_t pattern, size_t len, cbn_status_t& out_status);
};


class process_namespace : public refcountable
{
private:
	LinkedList<process *> process_list;
	uint32_t namespace_id;
	unsigned long process_limit;
	unsigned long nr_processes;
	Spinlock process_creation_lock;
	atomic<cbn_pid_t> curr_pid;
public:
	process_namespace(uint32_t id) : refcountable{0}, process_list{}, namespace_id{id},
		process_limit{0}, nr_processes{0}, process_creation_lock{}, curr_pid{1} {}
	~process_namespace();
	static shared_ptr<process_namespace> create_namespace(cbn_status_t& out_status);
	process *create_process_in_namespace(cbn_status_t& out_status);
	bool remove_process(process *p);
};

inline process *get_current_process()
{
	return get_current_thread()->owner;
}

#endif