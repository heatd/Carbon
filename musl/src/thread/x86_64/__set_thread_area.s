/* Copyright 2011-2012 Nicholas J. Kain, licensed under standard MIT license */
.text
.global __set_thread_area
.hidden __set_thread_area
.type __set_thread_area,@function
__set_thread_area:
	mov %rdi,%rsi           /* shift for syscall */
	movl $3,%edi      	/* tls_op::set_fs operation */
	movl $1,%eax          	/* set fs segment to */
	syscall                 /* cbn_tls_op(tls_op::set_fs, arg)*/
	ret
