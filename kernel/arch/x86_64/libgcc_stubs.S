.global __deregister_frame_info
.global __register_frame_info
.global _ITM_registerTMCloneTable
.global _ITM_deregisterTMCloneTab
_ITM_deregisterTMCloneTab:
_ITM_registerTMCloneTable:
__register_frame_info:
__deregister_frame_info:
	xor %eax, %eax
	ret
