.global __deregister_frame_info
.global __register_frame_info
.global _ITM_registerTMCloneTable
.global _ITM_deregisterTMCloneTab
.global __cxa_atexit
_ITM_deregisterTMCloneTab:
_ITM_registerTMCloneTable:
__register_frame_info:
__deregister_frame_info:
__cxa_atexit:
	# NOTE: We can do this with __cxa_atexit because we, as the kernel, will never exit!
	xor %eax, %eax
	ret
