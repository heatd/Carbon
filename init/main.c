#define _GNU_SOURCE
#include <unistd.h>

#include <sys/syscall.h>

int main(int argc, char **argv, char **envp) {syscall(SYS_cbn_open_sys_handle, "carbon.terminal0", 0x3);}