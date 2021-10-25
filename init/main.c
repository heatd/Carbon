#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/syscall.h>
#include <carbon/public/status.h>
#include <carbon/public/handle.h>
#include <carbon/public/vm.h>

#include <carbon/syscalls.h>

int main(int argc, char **argv, char **envp)
{
	cbn_handle_t ret = cbn_open_sys_handle("carbon.terminal0", 0x3);
	ret = cbn_open_sys_handle("carbon.terminal0", 0x3);
	cbn_handle_t _stdout = 0; 
	cbn_status_t res = cbn_duplicate_handle(CBN_INVALID_HANDLE, ret, -1,
		&_stdout, CBN_DUPLICATE_HANDLE_ALLOC_HANDLE);
	printf("stderr: %u\n", _stdout);
	/*if((int32_t) ret < 0)
	{
		__asm__ __volatile__("ud2");
	}*/

	printf("Hello World!\n");
	
	cbn_handle_t vmo_handle = 0;
	printf("Error %d\n", cbn_vmo_create(0x400000, &vmo_handle));
	printf("vmo handle %lx\n", vmo_handle);
	void *r = NULL;
	cbn_mmap(-1, vmo_handle, (void *) 0x7700000000, 0x400000, MAP_FLAG_FIXED, 0, MAP_PROT_WRITE, &r);
	printf("Result: %p\n", r);
	memset(r, 0xff, 0x400000);

	return 0;
}