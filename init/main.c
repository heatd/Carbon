#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/syscall.h>
#include <carbon/public/status.h>
#include <carbon/public/handle.h>
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

	size_t was_read = 0;
	cbn_write(_stdout, "Hello userspace!\n", strlen("Hello userspace!\n"), &was_read);
	printf("Hello World!\n");

}