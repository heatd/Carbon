.global __cxa_atexit
__cxa_atexit:
	# NOTE: We can do this with __cxa_atexit because we, as the kernel, will never exit!
	xor %eax, %eax
	ret
